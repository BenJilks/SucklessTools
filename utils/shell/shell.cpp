#include "shell.hpp"
#include "command.hpp"
#include <iostream>
#include <functional>
#include <fstream>
#include <sstream>
#include <termios.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>

Shell &Shell::the()
{
    static Shell shell;
	return shell;
}

Shell::Shell()
{
	auto *pw = getpwuid(getuid());

#ifdef __FreeBSD__
    home = "/usr" + std::string(pw->pw_dir);
#else
    home = std::string(pw->pw_dir);
#endif
    std::cout << home << "\n";
}

void Shell::handle_int_signal()
{
	if (fg_process == -1)
	{
		std::cout << "^C\n" << substitute_variables(get("PS1"));
        std::cout.flush();
		line = "";
		cursor = 0;
		return;
	}
	std::cout << "\n";
}

int Shell::foreground(pid_t pid)
{
	fg_process = pid;

	int status = -1;
	for (;;)
	{
		if (waitpid(pid, &status, WNOHANG) == pid)
			break;
	}

	fg_process = -1;
	return status;
}

char Shell::next_char()
{
    static struct termios old, current;
    
    tcgetattr(0, &old);
    current = old;
    current.c_lflag &= ~ICANON; // Disable buffered i/o
    current.c_lflag &= ~ECHO; // Disable echo
    tcsetattr(0, TCSANOW, &current);
    
    auto c = std::getchar();
    tcsetattr(0, TCSANOW, &old);
    
    return c;
}

void Shell::prompt()
{
	std::string ps1 = substitute_variables(get("PS1"));
	std::cout << ps1;

	line = "";
	cursor = 0;
	int history_index = -1;
	bool line_end = false;

	auto replace_line = [&](const std::string &with)
	{
		// Move to the start of the current line
		for (int i = 0; i < cursor; i++)
			std::cout << "\b";

		// Replace the current line with the new one
		std::cout << with << "\033[K";
		
		line = with;
		cursor = with.length();
	};

	auto insert = [&](const std::string &with)
	{
        if (cursor >= (int)line.length())
        {
            line += with;
            cursor += with.length();
            std::cout << with;
        }
        else
        {
            line.insert(cursor, with);
            cursor += with.length();
            
            std::cout << "\033[" << with.length() << "@";
            std::cout << with;
        }
	};

	auto message = [&](const std::string &msg)
	{
		std::cout << "\n" << msg << "\n";
		std::cout << ps1 << line;
	};

	while (!line_end)
	{
		char c = next_char();
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
                std::cout << "\n";
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
                        if (cursor < (int)line.length())
						{
							std::cout << line[cursor];
							cursor += 1;
						}
						break;
					case 'D': // Left
						if (cursor > 0)
						{
							cursor -= 1;
							std::cout << "\b";
						}
						break;
					
					case '1': // Home
						std::getchar(); // Skip ~
					case 'H': // Xterm Home
						for (int i = cursor - 1; i >= 0; --i)
							std::cout << "\b";
						cursor = 0;
						break;

					case '4': // End
						std::getchar(); // Skip ~
					case 'F': // Xterm end
                        for (int i = cursor; i < (int)line.length(); i++)
							std::cout << line[i];
						cursor = line.length();
						break;
				}
				break;
			}

			case '\b':
			case 127:
				if (cursor > 0)
				{
					std::cout << "\b \b\033[P";
					cursor -= 1;
					line.erase(cursor, 1);
				}
				break;

			default:
                insert(std::string(&c, 1));
				break;
            
		}
	}

	if (!line.empty())
    {
        command_history.push_back(line);
        for (auto &mod : modules)
            mod->hook_macro(line);
        exec_line(line);
    }
}

void Shell::exec_line(const std::string &line)
{
    Lexer lexer(line);
    lexer.hook_on_token = [&](Token &token, int index)
    {
        bool should_discard = false;
        for (auto &mod : modules)
        {
            if (mod->hook_on_token(lexer, token, index))
                should_discard = true;
        }

        return should_discard;
    };

    auto command = Command::parse(lexer);
	command->execute();
}

enum class State
{
	Default,
	Variable,
	Braced
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
				if (c == '{' && variable.empty())
				{
					state = State::Braced;
					break;
				}

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

			case State::Braced:
				if (c == '}')
				{
					result += simplify_path(get(variable));
					variable.clear();
					state = State::Default;
					break;
				}

				variable += c;
				break;
		}
	}

	return result;
}

std::string Shell::replace_all(
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

static void s_handle_sig_int(int)
{
	Shell::the().handle_int_signal();
}

void Shell::run()
{
	should_exit = false;
	signal(SIGINT, s_handle_sig_int);

	while (!should_exit)
	{
		prompt();
	}
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
	
    Lexer lexer(str_stream.str());
    auto script = Command::parse(lexer);
	script->execute();
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

