#include "command.hpp"
#include "lexer.hpp"
#include "shell.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>

#define READ 		0
#define WRITE 		1

std::map<std::string, BuiltIn> Command::built_ins;

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

	if (built_ins.find(command) != built_ins.end())
		return std::make_unique<CommandBuiltIn>(built_ins[command], arguments, assignments);

	return std::make_unique<CommandExec>(command, arguments, assignments);
}

std::unique_ptr<Command> Command::parse_command(Lexer &lexer)
{
	auto left = parse_exec(lexer);

	while (lexer.consume(Token::Type::Pipe))
	{
		auto right = parse_exec(lexer);
		left = std::make_unique<CommandPipe>(
			std::move(left), std::move(right));
	}

	return left;
}

std::unique_ptr<Command> Command::parse(const std::string &source)
{
	Lexer lexer(source);
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
	commands.push_back(std::move(command));
}

void CommandList::execute()
{
	for (auto &command : commands)
		command->execute_in_process();
}

CommandExec::CommandExec(std::string program, 
			std::vector<std::string> arguments, 
			std::vector<std::pair<std::string, std::string>> assignments)
	: program(program)
	, arguments(arguments)
	, assignments(assignments)
{
	// Build argument list
	raw_arguments.push_back(this->program.data());
	for (auto &argument : this->arguments)
		raw_arguments.push_back(argument.data());
	raw_arguments.push_back(nullptr);
	
	// Build envirment list
	for (auto &assignment : this->assignments)
	{
		auto variable = assignment.first + "=" + assignment.second;
		raw_assignments.push_back(variable.data());
	}
	raw_assignments.push_back(nullptr);
}

int Command::execute_in_process()
{
	if (!should_execute_in_process())
	{
		execute();
		return -1;
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		perror("Shell");
		return -1;
	}

	// In the child process
	if (pid == 0)
	{
		execute();
		exit(-1);
	}

	int status;
	waitpid(pid, &status, 0);

	return status;
}

void CommandExec::execute()
{
	if (execvpe(program.data(), 
		const_cast<char* const*>(raw_arguments.data()),
		const_cast<char* const*>(raw_assignments.data())) < 0)
	{
		perror("Shell");
	}
}

void CommandBuiltIn::execute()
{
	func(args, assignments);
}

void CommandSetEnv::execute()
{
	for (const auto &assignment : assignments)
		Shell::the().set(assignment.first, assignment.second);
}

void CommandPipe::execute()
{
	if (!left || !right)
		return;
	
	auto outer_pid = fork();
	if (outer_pid == -1)
	{
		perror("Shell: fork");
		return;
	}

	if (outer_pid != 0)
	{
		int status;
		waitpid(outer_pid, &status, 0);
		return;
	}

	int p[2];
	if (pipe(p) < 0)
	{
		perror("Shell: Pipe failed");
		exit(-1);
	}

	auto pid = fork();
	if (pid == -1)
	{
		perror("Shell");
		return;
	}
	
	if (pid == 0)
	{
		// Output process
		close(STDOUT_FILENO);
		dup(p[WRITE]);
	
		close(p[READ]);
		close(p[WRITE]);
		
		left->execute();
		exit(-1);
	}
	else
	{
		// Input process
		close(STDIN_FILENO);
		dup(p[READ]);
		
		close(p[WRITE]);
		close(p[READ]);
	
		right->execute();
		exit(-1);
	}
}

