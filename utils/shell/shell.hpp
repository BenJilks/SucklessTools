#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "modules/module.hpp"

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
	inline const std::string &get_home() const { return home; }

	void set(std::string env, const std::string &value);
	std::string get(std::string env);

	template<typename ModuleType>
	void add_module()
	{
		modules.push_back(std::make_unique<ModuleType>());
	}

	static std::string replace_all(std::string str, 
		const std::string &from, 
		const std::string &to);

private:
	void exec_line(const std::string &line);
	std::string substitute_variables(const std::string &);
    char next_char();

	std::string line;
	int cursor;

	std::vector<pid_t> bg_processes;
	pid_t fg_process { -1 };

	std::map<std::string, std::string> env_buffer;
	std::vector<std::string> command_history;
	std::vector<std::unique_ptr<Module>> modules;
	std::string home { "/" };
	bool should_exit { false };
	bool cancel_line { false };
};

