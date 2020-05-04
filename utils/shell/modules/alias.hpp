#pragma once
#include "module.hpp"
#include <map>
#include <vector>

class AliasModule : public Module
{
public:
	AliasModule();

	virtual bool hook_macro(std::string &line) override;

private:
	std::map<std::string, std::string> aliases;

};

