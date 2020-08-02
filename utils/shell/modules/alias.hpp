#pragma once
#include "module.hpp"
#include <map>
#include <vector>

class AliasModule : public Module
{
public:
	AliasModule();

    virtual bool hook_on_token(Lexer&, Token&, int index) override;

private:
	std::map<std::string, std::string> aliases;

};

