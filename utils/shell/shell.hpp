#pragma once
#include "modules/module.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <sys/types.h>

class Shell
{
public:
	Shell();

	static Shell &the();

	void prompt();
	void run();
	void run_script(const std::string &file_path);
	void exit();

	int foreground(pid_t);
	void handle_int_signal();

	std::string simplify_path(const std::string &path);
	std::string expand_path(const std::string &path);
	std::string directory_name(const std::string &path);
    inline const std::string &get_home() const { return m_home; }

	void set(std::string env, const std::string &value);
	std::string get(std::string env);

	template<typename ModuleType>
	void add_module()
	{
        m_modules.push_back(std::make_unique<ModuleType>());
	}

	static std::string replace_all(std::string str, 
		const std::string &from, 
		const std::string &to);

private:
    std::string substitute_variables(const std::string &);
    void exec_line(const std::string &line);
    char next_char();

    void load_history();
    void save_history();

    std::string m_line;
    int m_cursor;

    std::vector<pid_t> m_bg_processes;
    pid_t m_fg_process { -1 };

    std::vector<std::string> m_command_history;
    std::vector<std::unique_ptr<Module>> m_modules;
    std::string m_home { "/" };
    bool m_should_exit { false };
    bool m_cancel_line { false };
};
