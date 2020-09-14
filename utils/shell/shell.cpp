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
    m_home = "/usr" + std::string(pw->pw_dir);
#else
    m_home = std::string(pw->pw_dir);
#endif

    load_history();
}

void Shell::load_history()
{
    auto history_file_path = m_home + "/.shell_history";
    std::ifstream stream(history_file_path);
    std::string line;
    if (!stream.good())
        return;

    m_command_history.clear();
    while (std::getline(stream, line))
        m_command_history.push_back(line);
}

void Shell::save_history()
{
    auto history_file_path = m_home + "/.shell_history";
    std::ofstream stream(history_file_path);

    for (const auto &command : m_command_history)
        stream << command << "\n";
}

void Shell::handle_int_signal()
{
    if (m_fg_process == -1)
	{
		std::cout << "^C\n" << substitute_variables(get("PS1"));
        std::cout.flush();
        m_line = "";
        m_cursor = 0;
		return;
	}
	std::cout << "\n";
}

int Shell::foreground(pid_t pid)
{
    m_fg_process = pid;

	int status = -1;
	waitpid(pid, &status, 0);

    m_fg_process = -1;
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
    std::cout.flush();

    m_line = "";
    m_cursor = 0;
	int history_index = -1;
	bool line_end = false;

	auto replace_line = [&](const std::string &with)
	{
		// Move to the start of the current line
        for (int i = 0; i < m_cursor; i++)
			std::cout << "\b";

		// Replace the current line with the new one
		std::cout << with << "\033[K";
        std::cout.flush();

        m_line = with;
        m_cursor = with.length();
	};

	auto insert = [&](const std::string &with)
	{
        if (m_cursor >= (int)m_line.length())
        {
            m_line += with;
            m_cursor += with.length();
            std::cout << with;
        }
        else
        {
            m_line.insert(m_cursor, with);
            m_cursor += with.length();
            
            std::cout << "\033[" << with.length() << "@";
            std::cout << with;
        }
        std::cout.flush();
    };

	auto message = [&](const std::string &msg)
	{
		std::cout << "\n" << msg << "\n";
        std::cout << ps1 << m_line;
        std::cout.flush();
    };

	while (!line_end)
	{
		char c = next_char();
		bool has_module_hook = false;

        for (const auto &mod : m_modules)
		{
			if (mod->hook_input(c, 
                m_line, m_cursor,
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
                        if (history_index < (int)m_command_history.size() - 1)
						{
							history_index += 1;
                            auto command = m_command_history[m_command_history.size() - history_index - 1];
							replace_line(command);
						}
						break;
					case 'B': // Down
						if (history_index >= 0)
						{
							history_index -= 1;
							auto command = history_index >= 0 
                                ? m_command_history[m_command_history.size() - history_index - 1]
								: "";
							replace_line(command);
						}
						break;

					// Navigation
					case 'C': // Right
                        if (m_cursor < (int)m_line.length())
						{
                            std::cout << m_line[m_cursor];
                            std::cout.flush();
                            m_cursor += 1;
                        }
						break;
					case 'D': // Left
                        if (m_cursor > 0)
						{
                            m_cursor -= 1;
                            std::cout << "\b";
                            std::cout.flush();
                        }
						break;
					
					case '1': // Home
						std::getchar(); // Skip ~
					case 'H': // Xterm Home
                        for (int i = m_cursor - 1; i >= 0; --i)
							std::cout << "\b";
                        std::cout.flush();
                        m_cursor = 0;
						break;

					case '4': // End
						std::getchar(); // Skip ~
					case 'F': // Xterm end
                        for (int i = m_cursor; i < (int)m_line.length(); i++)
                            std::cout << m_line[i];
                        std::cout.flush();
                        m_cursor = m_line.length();
						break;
				}
				break;
			}

			case '\b':
			case 127:
                if (m_cursor > 0)
				{
					std::cout << "\b \b\033[P";
                    std::cout.flush();
                    m_cursor -= 1;
                    m_line.erase(m_cursor, 1);
				}
				break;

			default:
                insert(std::string(&c, 1));
				break;
            
		}
	}

    if (!m_line.empty())
    {
        m_command_history.push_back(m_line);
        save_history();

        for (auto &mod : m_modules)
            mod->hook_macro(m_line);
        exec_line(m_line);
    }
}

void Shell::exec_line(const std::string &line)
{
    Lexer lexer(line);
    lexer.hook_on_token = [&](Token &token, int index)
    {
        bool should_discard = false;
        for (auto &mod : m_modules)
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
    return replace_all(path, m_home, "~");
}

std::string Shell::expand_path(const std::string &path)
{
    return replace_all(path, "~", m_home);
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
    m_should_exit = false;
	signal(SIGINT, s_handle_sig_int);

    while (!m_should_exit)
	{
		prompt();
	}
}

void Shell::exit()
{
    m_should_exit = true;
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
    setenv(name.c_str(), value.c_str(), 1);
}

std::string Shell::get(std::string name)
{
	auto env = getenv(name.data());
	if (!env)
		return "";
	
	return env;
}
