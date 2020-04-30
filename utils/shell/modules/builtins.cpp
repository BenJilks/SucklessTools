#include "builtins.hpp"
#include "../command.hpp"
#include "../shell.hpp"
#include <iostream>
#include <unistd.h>
#include <limits.h>

BuiltInsModule::BuiltInsModule()
{
	Command::register_built_in("cd", [](const std::vector<std::string> &args)
	{
		if (args.size() == 0)
		{
			std::cout << "FIXME: Go to home\n";
			return;
		}

		if (args.size() > 1)
		{
			std::cout << "cd: Too many arguments\n";
			return;
		}
		
		auto dir = args[0];
		chdir(dir.c_str());

		char cwd[PATH_MAX];
		getcwd(cwd, sizeof(cwd));
		Shell::the().set("PWD", cwd);
	});

	Command::register_built_in("exit", [](const std::vector<std::string>&)
	{
		Shell::the().exit();
	});
}

