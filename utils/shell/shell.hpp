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

	void disable_echo();
	void enable_echo();

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

private:
	void exec_line(const std::string &line);
	std::string substitute_variables(const std::string &);

	std::map<std::string, std::string> env_buffer;
	std::vector<std::string> command_history;
	std::vector<std::unique_ptr<Module>> modules;
	std::string home;
	bool should_exit;
};

