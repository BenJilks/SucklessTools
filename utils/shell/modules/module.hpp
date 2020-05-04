#pragma once
#include <string>
#include <functional>

class Module
{
public:
	Module() {}

	virtual bool hook_input(char c, 
		const std::string &line, int cursor,
		std::function<void(const std::string &)> insert,
		std::function<void(const std::string &)> replace,
		std::function<void(const std::string &)> message) 
	{ return false; }

	virtual bool hook_macro(std::string &line) { return false; }

	static const std::string_view name() { return "Module"; }
};

