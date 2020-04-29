#include "shell.hpp"
#include "command.hpp"
#include <iostream>

Shell shell;

Shell &Shell::the()
{
	return shell;
}

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

void Shell::set(std::string name, const std::string &value)
{
	env_buffer[name] = name + "=" + value;
	putenv(env_buffer[name].data());
}

std::string Shell::get(std::string name)
{
	auto env = getenv(name.data());
	if (!env)
		return "";
	
	return env;
}

