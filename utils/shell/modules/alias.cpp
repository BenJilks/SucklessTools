#include "alias.hpp"
#include "../command.hpp"
#include <iostream>

AliasModule::AliasModule()
{
	Command::register_built_in("alias", [&](
		const std::vector<std::string> &arguments,
		const std::vector<std::pair<std::string, std::string>>& assignments) -> int
	{
		if (arguments.size() == 0 && assignments.size() == 0)
		{
			for (const auto &value : aliases)
				std::cout << "alias " << value.first << "='" << value.second << "'\n";
			return 0;
		}

		bool did_fail = false;
		for (const auto &argument : arguments)
		{
			if (aliases.find(argument) == aliases.end())
			{
				did_fail = true;
				std::cout << "Shell: alias: " << argument << ": not found\n";
				continue;
			}

			std::cout << "alias " << argument << "='" << aliases[argument] << "'\n";
		}

		for (const auto &assignment : assignments)
			aliases[assignment.first] = assignment.second;

		return did_fail ? -1 : 0;
	});
}

bool AliasModule::hook_on_token(Lexer &lexer, Token &token, int index)
{
    if (index != 0)
        return false;

    for (const auto &[alias, value] : aliases)
    {
        if (token.data == alias)
        {
            lexer.insert_string(value);
            return true;
        }
    }

    return false;
}
