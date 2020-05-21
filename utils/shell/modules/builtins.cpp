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
	Shell::the().set("DIRNAME", Shell::the().directory_name(Shell::the().simplify_path(cwd)));
}

BuiltInsModule::BuiltInsModule()
{
	update_env_variables();

	Command::register_built_in("cd", [](
		const std::vector<std::string> &args,
		const std::vector<std::pair<std::string, std::string>>&) -> int
	{
		if (args.size() == 0)
		{
			auto home = Shell::the().get_home();
			chdir(home.c_str());
			Shell::the().set("PWD", home);
			Shell::the().set("DIRNAME", "~");
			return 0;
		}

		if (args.size() > 1)
		{
			std::cerr << "cd: Too many arguments\n";
			return -1;
		}
		
		auto dir = Shell::the().expand_path(args[0]);
		if (chdir(dir.c_str()) < 0)
		{
			perror("Shell: cd");
			return -1;
		}
		update_env_variables();
		return 0;
	});

	Command::register_built_in("exit", [](
		const std::vector<std::string>&,
		const std::vector<std::pair<std::string, std::string>>&) -> int
	{
		Shell::the().exit();
		return 0;
	});

	Command::register_built_in("export", [](
		const std::vector<std::string>&, 
		const std::vector<std::pair<std::string, std::string>> &assignments) -> int
	{
		for (const auto &assignment : assignments)
			Shell::the().set(assignment.first, assignment.second);

		return 0;
	});
}

