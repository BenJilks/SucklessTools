#pragma once
#include <string>
#include <functional>

class Module
{
public:
	Module() {}

	virtual void hook_init() {}
	virtual bool hook_parse(const std::string &line) { return false; }
	virtual bool hook_input(char c, 
		const std::string &line, int cursor,
		std::function<void(const std::string &)> insert,
		std::function<void(const std::string &)> replace,
		std::function<void(const std::string &)> message) 
	{ return false; } 

	static const std::string_view name() { return "Module"; }
};

