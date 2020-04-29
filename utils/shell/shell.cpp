#include "shell.hpp"
#include "command.hpp"
#include <iostream>

Shell::Shell()
{
}

void Shell::prompt()
{
	std::string ps1 = get("PS1");
	std::cout << ps1;

	std::string line;
	std::getline(std::cin, line);
	exec_line(line);
}

enum class State
{
	Command,
	Argument
};

void Shell::exec_line(const std::string &line)
{
	auto command = Command::parse(line);
	command->execute_in_process();
}

void Shell::run()
{
	for (;;)
	{
		prompt();
	}
}

void Shell::set(std::string name, std::string value)
{
	env[name] = value;
}

std::string Shell::get(std::string name)
{
	return env[name];
}

