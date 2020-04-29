#include "command.hpp"
#include "lexer.hpp"
#include "shell.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>

#define READ 		0
#define WRITE 		1

static std::unique_ptr<Command> parse_exec(Lexer &lexer)
{
	auto command_token = lexer.consume(Token::Type::Name);
	if (!command_token)
		return nullptr;

	std::string command = command_token->data;
	std::vector<std::string> arguments;
	bool has_next = true;
	while (has_next)
	{
		auto peek = lexer.peek();
		if (!peek)
			break;

		switch (peek->type)
		{
			case Token::Type::Variable: 
				arguments.push_back(Shell::the().get(lexer.consume(Token::Type::Variable)->data));
				break;

			case Token::Type::Name: 
				arguments.push_back(lexer.consume(Token::Type::Name)->data);
				break;

			default: has_next = false; break;
		}
	}

	return std::make_unique<CommandExec>(command, arguments);
}

std::unique_ptr<Command> Command::parse(const std::string &source)
{
	Lexer lexer(source);
	auto left = parse_exec(lexer);

	while (lexer.consume(Token::Type::Pipe))
	{
		auto right = parse_exec(lexer);
		left = std::make_unique<CommandPipe>(
			std::move(left), std::move(right));
	}

	return left;
}

int Command::execute_in_process()
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
		execute();
		exit(0);
	}

	int status;
	waitpid(pid, &status, 0);

	return status;
}

void CommandExec::execute()
{
	const char **raw_arguments = new const char*[arguments.size() + 2];

	raw_arguments[0] = program.data();
	for (int i = 0; i < arguments.size(); i++)
		raw_arguments[i + 1] = arguments[i].data();
	raw_arguments[arguments.size() + 1] = nullptr;

	if (execvp(program.data(), (char**)raw_arguments) < 0)
		perror("Shell");

	delete[] raw_arguments;
}

void CommandPipe::execute()
{
	if (!left || !right)
		return;

	auto outer_pid = fork();
	if (outer_pid == -1)
	{
		perror("Shell");
		return;
	}	

	if (outer_pid == 0)
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
			exit(0);
		}
		else
		{
			// Input process
			close(STDIN_FILENO);
			dup(p[READ]);
			
			close(p[WRITE]);
			close(p[READ]);
	
			right->execute();
			exit(0);
		}
	}

	int status;
	waitpid(outer_pid, &status, 0);
}

