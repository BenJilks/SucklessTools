#pragma once
#include <string>
#include <map>
#include <vector>

class Shell
{
public:
	Shell();

	static Shell &the();

	void prompt();
	void run();

	void set(std::string env, const std::string &value);
	std::string get(std::string env);

private:
	void exec_line(const std::string &line);
	void exec_command(const std::string command, const std::vector<std::string> arguments);

	std::map<std::string, std::string> env_buffer;
	std::vector<std::string> command_history;

};
