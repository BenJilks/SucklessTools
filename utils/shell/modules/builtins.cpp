#include "builtins.hpp"
#include "../command.hpp"
#include "../shell.hpp"
#include <iostream>
#include <unistd.h>
#include <limits.h>

static void update_env_variables()
{
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	Shell::the().set("PWD", cwd);
	Shell::the().set("DIRNAME", Shell::the().directory_name(cwd));
}

BuiltInsModule::BuiltInsModule()
{
	update_env_variables();

	Command::register_built_in("cd", [](
		const std::vector<std::string> &args,
		const std::vector<std::pair<std::string, std::string>>&)
	{
		if (args.size() == 0)
		{
			auto home = Shell::the().get_home();
			chdir(home.c_str());
			Shell::the().set("PWD", home);
			Shell::the().set("DIRNAME", "~");
			return;
		}

		if (args.size() > 1)
		{
			std::cout << "cd: Too many arguments\n";
			return;
		}
		
		auto dir = Shell::the().expand_path(args[0]);
		chdir(dir.c_str());
		update_env_variables();
	});

	Command::register_built_in("exit", [](
		const std::vector<std::string>&,
		const std::vector<std::pair<std::string, std::string>>&)
	{
		Shell::the().exit();
	});

	Command::register_built_in("export", [](
		const std::vector<std::string>&, 
		const std::vector<std::pair<std::string, std::string>> &assignments)
	{
		for (const auto &assignment : assignments)
			Shell::the().set(assignment.first, assignment.second);
	});
}

