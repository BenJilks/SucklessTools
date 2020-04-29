#include "shell.hpp"
#include "command.hpp"
#include <iostream>
#include <termios.h>

Shell shell;

Shell &Shell::the()
{
	return shell;
}

Shell::Shell()
{
	struct termios config;
	if (tcgetattr(0, &config) < 0)
		perror("tcgetattr()");
	
	config.c_lflag &= ~ICANON;
	config.c_lflag &= ~ECHO;
	config.c_cc[VMIN] = 1;
	config.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &config) < 0)
		perror("tcsetattr()");
}

void Shell::prompt()
{
	std::string ps1 = get("PS1");
	std::cout << ps1;

	std::string line = "";
	int cursor = 0;
	bool line_end = false;
	while (!line_end)
	{
		char c = std::getchar();

		switch(c)
		{
			case '\n':
				line_end = true;
				break;

			case '\033':
				std::getchar(); // Skip [
				switch(std::getchar())
				{
					case 'A': // Up
						break;
					case 'B': // Down
						break;
					case 'C': // Right
						if (cursor < line.length())
						{
							cursor += 1;
							std::cout << "\033[C";
						}
						break;
					case 'D': // Left
						if (cursor > 0)
						{
							cursor -= 1;
							std::cout << "\033[D";
						}
						break;
				}
				break;

			case '\b':
			case 127:
				if (!line.empty())
				{
					std::cout << "\033[D";	
					cursor -= 1;
					
					for (int i = cursor + 1; i < line.length(); i++)
						std::cout << line[i];
					std::cout << " ";
					for (int i = cursor + 1; i < line.length(); i++)
						std::cout << "\033[D";
					std::cout << "\033[D";

					line.erase(cursor, 1);
				}
				break;

			default:
				line.insert(cursor, sizeof(char), c);
				for (int i = cursor; i < line.length(); i++)
					std::cout << line[i];
				for (int i = cursor; i < line.length(); i++)
					std::cout << "\033[D";

				cursor += 1;
				std::cout << c;
				break;

		}
	}

	std::cout << std::endl;
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

