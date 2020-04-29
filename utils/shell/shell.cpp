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
	// Disable echo so we can use our own custom prompt
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
	int history_index = -1;
	bool line_end = false;

	auto replace_line = [&](std::string with)
	{
		// Move the the start of the current line
		for (int i = 0; i < cursor; i++)
			std::cout << "\033[D";

		// Replace the current line with the new one
		std::cout << with;
		
		for (int i = with.length(); i < (int)line.length(); i++)
			std::cout << " ";
		for (int i = with.length(); i < (int)line.length(); i++)
			std::cout << "\033[D";
		
		line = with;
		cursor = with.length();
	};

	while (!line_end)
	{
		char c = std::getchar();

		switch(c)
		{
			case '\n':
				line_end = true;
				break;

			case '\033':
			{
				std::getchar(); // Skip [
				char action_char = std::getchar();
				switch(action_char)
				{
					// History
					case 'A': // Up
						if (history_index < (int)command_history.size() - 1)
						{
							history_index += 1;
							auto command = command_history[command_history.size() - history_index - 1];
							replace_line(command);
						}
						break;
					case 'B': // Down
						if (history_index >= 0)
						{
							history_index -= 1;
							auto command = history_index >= 0 
								? command_history[command_history.size() - history_index - 1] 
								: "";
							replace_line(command);
						}
						break;

					// Navigation
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
					
					case '1': // Home
						std::getchar(); // Skip ~
					case 'H': // Xterm Home
						for (int i = cursor - 1; i >= 0; --i)
							std::cout << "\033[D";
						cursor = 0;
						break;

					case '4': // End
						std::getchar(); // Skip ~
					case 'F': // Xterm end
						for (int i = cursor; i < line.length(); i++)
							std::cout << "\033[C";
						cursor = line.length();
						break;
					defualt:
						std::cout << "\033[" << action_char;
						cursor += 3;
						break;
				}
				break;
			}

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
	command_history.push_back(line);
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

