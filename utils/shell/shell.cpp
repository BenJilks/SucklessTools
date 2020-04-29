#include "shell.hpp"
#include "command.hpp"
#include <iostream>
#include <filesystem>
#include <functional>
#include <termios.h>
namespace fs = std::filesystem;

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
			
			case '\t':
			{
				auto patch = auto_complete(line, cursor);
				if (patch.empty())
				{				
					std::cout << ps1 << line;
					for (int i = cursor; i < line.length(); i++)
						std::cout << "\033[D";
					break;
				}

				std::cout << patch;
				line += patch;
				cursor += patch.length();
				break;
			}
			
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

std::string Shell::auto_complete(const std::string &line, int cursor)
{
	int word_start = cursor;
	while (word_start > 0)
	{
		char c = line[word_start - 1];
		if (std::isspace(c))
			break;

		word_start -= 1;
	}

	std::string_view curr_word(
		line.data() + word_start, 
		cursor - word_start);

	// Find all options for word
	auto options = find_completions(
		curr_word, word_start == 0);

	std::string patch = "";
	for (;;)
	{
		char next_char = 0;
		for (const auto &option : options)
		{
			int index = curr_word.length() + patch.length();
			if (index > option.length())
			{
				next_char = 0;
				break;
			}

			char c = option[index];
			if (next_char == 0)
				next_char = c;

			if (c != next_char)
			{
				next_char = 0;
				break;
			}

			next_char = c;
		}

		if (!next_char)
			break;

		patch += next_char;
	}

	if (patch.empty())
	{
		if (options.size() == 1)
			return " ";

		std::cout << "\n";
		for (const auto &option : options)
			std::cout << option << " ";
		std::cout << "\n";
	}

	return patch;
}

std::vector<std::string> Shell::find_completions(
	const std::string_view &start, bool search_path)
{
	std::vector<std::string> options;
	
	int last_slash_index = 0;
	for (int i = start.length() - 1; i >= 0; i--)
	{
		if (start[i] == '/')
		{
			last_slash_index = i + 1;
			break;
		}
	}

	auto path = std::string(start.substr(0, last_slash_index));
	auto local_path = "./" + std::string(path);

	// If the path begins with a slash, it's an absolute path
	if (!path.empty())
	{
		if (path[0] == '/')
			local_path = path;
	}

	// Look through local paths
	for (const auto &entry : fs::directory_iterator(local_path))
	{
		auto filename = std::string(path) + std::string(entry.path().filename());
		if (filename.rfind(start, 0) == 0)
			options.push_back(filename);
	}

	// If search path is enabled, look through all the paths in the envirement $PATH
	// variable for options
	if (search_path)
	{
		auto env_path = get("PATH");
		std::string buffer;
		
		for (int i = 0; i < env_path.length(); i++)
		{
			char c = env_path[i];
			if (c == ':')
			{
				for (const auto &entry : fs::directory_iterator(buffer))
				{
					auto filename = std::string(entry.path().filename());
					if (filename.rfind(start, 0) == 0)
						options.push_back(filename);
				}

				buffer = "";
				continue;
			}

			buffer += c;
		}
	}

	return options;
}

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

