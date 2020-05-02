#include "shell.hpp"
#include "command.hpp"
#include <iostream>
#include <functional>
#include <fstream>
#include <sstream>
#include <termios.h>
#include <pwd.h>
#include <unistd.h>

Shell shell;

Shell &Shell::the()
{
	return shell;
}

Shell::Shell()
{
	auto *pw = getpwuid(getuid());
	home = std::string(pw->pw_dir);
}

void Shell::disable_echo()
{
	struct termios config;
	if (tcgetattr(0, &config) < 0)
		perror("tcgetattr()");
	
	config.c_lflag &= ~ICANON;
	config.c_lflag &= ~ECHO;
	if (tcsetattr(0, TCSANOW, &config) < 0)
		perror("tcsetattr()");
}

void Shell::enable_echo()
{
	struct termios config;
	if (tcgetattr(0, &config) < 0)
		perror("tcgetattr()");
	
	config.c_lflag |= ICANON;
	config.c_lflag |= ECHO;
	if (tcsetattr(0, TCSANOW, &config) < 0)
		perror("tcsetattr()");
}

void Shell::prompt()
{
	std::string ps1 = substitute_variables(get("PS1"));
	std::cout << ps1;

	std::string line = "";
	int cursor = 0;
	int history_index = -1;
	bool line_end = false;

	auto replace_line = [&](const std::string &with)
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

	auto insert = [&](const std::string &with)
	{
		for (char c : with)
		{
			line.insert(cursor, sizeof(char), c);
			for (int i = cursor; i < line.length(); i++)
				std::cout << line[i];
			for (int i = cursor; i < line.length(); i++)
				std::cout << "\033[D";
	
			cursor += 1;
			std::cout << c;
		}
	};

	auto message = [&](const std::string &msg)
	{
		std::cout << "\n" << msg << "\n";
		std::cout << ps1 << line;
		for (int i = cursor; i < line.length(); i++)
			std::cout << "\033[D";
	};

	while (!line_end)
	{
		char c = std::getchar();
		bool has_module_hook = false;

		for (const auto &mod : modules)
		{
			if (mod->hook_input(c, 
				line, cursor,
				insert, replace_line, message))
			{
				has_module_hook = true;
				break;
			}
		}

		if (has_module_hook)
			continue;

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

void Shell::exec_line(const std::string &line)
{
	enable_echo();
	auto command = Command::parse(line);
	command->execute_in_process();
	disable_echo();
}

enum class State
{
	Default,
	Variable
};

std::string Shell::substitute_variables(const std::string &str)
{
	std::string result, variable;
	auto state = State::Default;

	for (char c : str)
	{
		switch (state)
		{
			case State::Default:
				if (c == '$')
				{
					state = State::Variable;
					break;
				}

				result += c;
				break;

			case State::Variable:
				if (!std::isalnum(c) && c != '_')
				{
					if (variable.empty())
						result += '$';
					else
						result += simplify_path(get(variable));
					
					variable.clear();
					state = State::Default;
					result += c;
					break;
				}

				variable += c;
				break;
		}
	}

	return result;
}

static std::string replace_all(
	std::string str, 
	const std::string &from, 
	const std::string &to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}

	return str;
}

std::string Shell::simplify_path(const std::string &path)
{
	return replace_all(path, home, "~");
}

std::string Shell::expand_path(const std::string &path)
{
	return replace_all(path, "~", home);
}

std::string Shell::directory_name(const std::string &path)
{
	int last_slash_index = 0;
	for (int i = path.length() - 1; i >= 0; --i)
	{
		if (path[i] == '/')
		{
			last_slash_index = i + 1;
			break;
		}
	}

	if (last_slash_index == 1 && path.length() == 1)
		return "/";

	return path.substr(last_slash_index, path.length() - last_slash_index);
}

void Shell::run()
{
	disable_echo();
	should_exit = false;

	while (!should_exit)
	{
		prompt();
	}

	enable_echo();
}

void Shell::exit()
{
	should_exit = true;
}

void Shell::run_script(const std::string &file_path)
{
	std::ifstream stream(file_path);
	std::stringstream str_stream;
	str_stream << stream.rdbuf();
	
	auto script = Command::parse(str_stream.str());
	script->execute_in_process();
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

