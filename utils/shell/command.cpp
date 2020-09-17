#include "command.hpp"
#include "lexer.hpp"
#include "shell.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <sstream>
#include <cassert>

#define READ 		0
#define WRITE 		1

std::map<std::string, BuiltIn> Command::s_built_ins;

std::pair<std::string, std::string> Command::parse_assignment(Lexer &lexer)
{
	auto token = lexer.consume(Token::Type::VariableAssignment);
	if (!token)
		return std::make_pair("", "");

	auto data = token->data;
	bool has_name = false;
	std::string name, value;
	
	for (const auto &c : data)
	{
		if (c == '=')
		{
			has_name = true;
			continue;
		}

		if (!has_name)
			name += c;
		else
			value += c;
	}

	return std::make_pair(name, value);
}

class StdOutCapture
{
public:
	StdOutCapture()
	{
		_stdout = std::cout.rdbuf(stream.rdbuf());
	}

	inline const std::string str() const { return stream.str(); }

	~StdOutCapture()
	{
		std::cout.rdbuf(_stdout);
	}

private:
	std::ostringstream stream;
	std::streambuf *_stdout;

};

std::unique_ptr<Command> Command::parse_exec(Lexer &lexer)
{
	std::string command;
	std::vector<std::string> arguments;
	std::vector<std::pair<std::string, std::string>> assignments;

	bool has_command = false;
	bool has_next = true;
	while (has_next)
	{
		auto peek = lexer.peek();
		if (!peek)
			break;

		switch (peek->type)
		{
			case Token::Type::Variable:
			{
				auto value = Shell::the().get(lexer.consume(Token::Type::Variable)->data);
				if (!has_command)
				{
					command = value;
					has_command = true;
					break;
				}
				
				arguments.push_back(value);
				break;
			}
			
			case Token::Type::VariableAssignment:
				assignments.push_back(parse_assignment(lexer));
				break;

			case Token::Type::SubCommand:
			{
				auto source = lexer.consume(Token::Type::SubCommand)->data;
                Lexer lexer(source);
                auto sub_command = parse(lexer);

				StdOutCapture capture;
				sub_command->execute();
				arguments.push_back(capture.str());
				break;
			}

			case Token::Type::Name: 
				if (!has_command)
				{
					command = lexer.consume(Token::Type::Name)->data;
					has_command = true;
					break;
				}

				arguments.push_back(lexer.consume(Token::Type::Name)->data);
				break;

			default: has_next = false; break;
		}
	}

	if (!has_command)
	{
		if (assignments.size() > 0)
			return std::make_unique<CommandSetEnv>(assignments);
		return nullptr;
	}

    if (s_built_ins.find(command) != s_built_ins.end())
        return std::make_unique<CommandBuiltIn>(s_built_ins[command], arguments, assignments);

    return std::make_unique<CommandExec>(command, arguments, assignments);
}

std::unique_ptr<Command> Command::parse_command(Lexer &lexer)
{
	auto left = parse_exec(lexer);

	auto peek = lexer.peek();
	while (peek && (
		peek->type == Token::Type::Pipe || 
		peek->type == Token::Type::And ||
		peek->type == Token::Type::With ||
        peek->type == Token::Type::Or ||
        peek->type == Token::Type::Redirect ||
        peek->type == Token::Type::RedirectErr ||
        peek->type == Token::Type::RedirectAll ||
        peek->type == Token::Type::RedirectAppend ||
        peek->type == Token::Type::RedirectErrAppend ||
        peek->type == Token::Type::RedirectAllAppend))
	{
		lexer.consume(peek->type);
		
		switch (peek->type)
		{
		case Token::Type::Pipe:
        {
            auto right = parse_exec(lexer);
            left = std::make_unique<CommandPipe>(
				std::move(left), std::move(right));
			break;
        }

		case Token::Type::And:
        {
            auto right = parse_exec(lexer);
            left = std::make_unique<CommandAnd>(
				std::move(left), std::move(right));
			break;
        }

		case Token::Type::With:
        {
            auto right = parse_exec(lexer);
            left = std::make_unique<CommandWith>(
				std::move(left), std::move(right));
			break;
        }

		case Token::Type::Or:
        {
            auto right = parse_exec(lexer);
            left = std::make_unique<CommandOr>(
				std::move(left), std::move(right));
			break;
        }

#define REDIRECT(type, mode, capture) \
    case Token::Type::type: \
    { \
        auto right = lexer.consume(Token::Type::Name); \
        assert (right); \
        left = std::make_unique<CommandRedirect>( \
            std::move(left), right->data, \
            CommandRedirect::Mode::mode, CommandRedirect::Capture::capture); \
        break; \
    }

        REDIRECT(Redirect, Override, StdOut);
        REDIRECT(RedirectErr, Override, StdErr);
        REDIRECT(RedirectAll, Override, All);
        REDIRECT(RedirectAppend, Append, StdOut);
        REDIRECT(RedirectErrAppend, Append, StdErr);
        REDIRECT(RedirectAllAppend, Append, All);

        default:
            assert (false);
		}

		peek = lexer.peek();
	}

	return left;
}

std::unique_ptr<Command> Command::parse(Lexer &lexer)
{	
	auto command_list = std::make_unique<CommandList>();

	for (;;)
	{
		auto command = parse_command(lexer);
		if (command)
			command_list->add(command);

		if (!lexer.consume(Token::Type::EndCommand))
			break;
	}

	return command_list;
}

void CommandList::add(std::unique_ptr<Command> &command)
{
    m_commands.push_back(std::move(command));
}

int CommandList::execute()
{
	int last_status = 0;
    for (auto &command : m_commands)
		last_status = command->execute();
	
	return last_status;
}

CommandExec::CommandExec(std::string program, 
			std::vector<std::string> arguments, 
			std::vector<std::pair<std::string, std::string>> assignments)
    : m_program(program)
    , m_arguments(arguments)
    , m_assignments(assignments)
{
	// Build argument list
    m_raw_arguments.push_back(this->m_program.data());
    for (auto &argument : this->m_arguments)
        m_raw_arguments.push_back(argument.data());
    m_raw_arguments.push_back(nullptr);
	
	// Build envirment list
    for (auto &assignment : this->m_assignments)
	{
		auto variable = assignment.first + "=" + assignment.second;
        m_raw_assignments.push_back(variable.data());
	}
    m_raw_assignments.push_back(nullptr);
}

int CommandExec::execute()
{
	pid_t pid = fork();
	if (pid == -1)
	{
		perror("Shell");
		return -1;
	}

	// In the child process
	if (pid == 0)
	{
		execute_and_exit();
		exit(-1);
	}

	return Shell::the().foreground(pid);
}

void CommandExec::execute_and_exit()
{
    if (execvp(m_program.data(),
        const_cast<char* const*>(m_raw_arguments.data())))
	{
		perror("Shell");
	}

	exit(-1);
}

int CommandBuiltIn::execute()
{
    return m_func(m_args, m_assignments);
}

int CommandSetEnv::execute()
{
    for (const auto &assignment : m_assignments)
		Shell::the().set(assignment.first, assignment.second);
	
	return 0;
}

int CommandPipe::execute()
{
    if (!m_left || !m_right)
        return -1;

    auto pid = fork();
	if (pid == -1)
	{
        perror("fork");
		return -1;
	}

	if (pid == 0)
	{
		execute_and_exit();
		exit(-1);
	}

	return Shell::the().foreground(pid);
}

void CommandPipe::execute_and_exit()
{
	int p[2];
	if (pipe(p) < 0)
	{
		perror("Shell: Pipe failed");
		exit(-1);
	}

	auto pid = fork();
	if (pid == -1)
	{
        perror("fork");
		return;
	}
	
	if (pid == 0)
	{
		// Output process
		close(STDOUT_FILENO);
		dup(p[WRITE]);
	
		close(p[READ]);
		close(p[WRITE]);
		
        m_left->execute_and_exit();
		exit(-1);
	}
	else
	{
		// Input process
		close(STDIN_FILENO);
		dup(p[READ]);
		
		close(p[WRITE]);
		close(p[READ]);
	
        m_right->execute_and_exit();
		exit(-1);
	}
}

int CommandRedirect::execute()
{
    int p[2];
    if (pipe(p) < 0)
    {
        perror("pipe");
        return -1;
    }

    auto pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    if (pid == 0)
    {
        // Output process
        if (m_capture == Capture::StdOut || m_capture == Capture::All)
            dup2(p[WRITE], STDOUT_FILENO);
        if (m_capture == Capture::StdErr || m_capture == Capture::All)
            dup2(p[WRITE], STDERR_FILENO);

        close(p[READ]);
        close(p[WRITE]);

        m_left->execute_and_exit();
        exit(-1);
    }

    // Input process
    close(p[WRITE]);

    static char buffer[1024];
    auto output = fopen(m_right.c_str(), m_mode == Mode::Append ? "ab" : "wb");
    for (;;)
    {
        auto len_read = read(p[READ], buffer, sizeof(buffer));
        if (len_read == 0)
            break;

        fwrite(buffer, sizeof(char), len_read, output);
    }

    fclose(output);
    close(p[READ]);
    return 0;
}

int CommandAnd::execute()
{
    if (!m_left || !m_right)
		return -1;
	
    int left_status = m_left->execute();
	if (left_status == 0)
        return m_right->execute();
	
	return -1;
}

int CommandOr::execute()
{
    if (!m_left || !m_right)
		return -1;
	
    int left_status = m_left->execute();
	if (left_status != 0)
        return m_right->execute();
	
	return 0;
}

int CommandWith::execute()
{
    if (!m_left)
		return -1;

	auto pid = fork();
	if (pid == -1)
	{
		perror("Shell: fork");
		return -1;
	}

	// Child process
	if (pid == 0)
	{
        m_left->execute_and_exit();
		exit(-1);
	}

    if (!m_right)
		return 0;
	
    return m_right->execute();
}

