#pragma once
#include "module.hpp"

class AutoCompleteModule : public Module
{
public:
	AutoCompleteModule() {}

	virtual bool hook_input(char c, 
		const std::string &line, int cursor,
		std::function<void(const std::string &)> insert,
		std::function<void(const std::string &)> replace,
		std::function<void(const std::string &)> message) override;

	static const std::string_view name() { return "Auto Complete"; }

private:
	std::vector<std::string> find_completions(const std::string_view &start, bool search_path);

};

