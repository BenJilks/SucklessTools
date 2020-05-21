#include "shell.hpp"
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include "modules/autocomplete.hpp"
#include "modules/builtins.hpp"
#include "modules/alias.hpp"

void run_if_exists(const std::string &file_path)
{
	if (fs::exists(file_path))
		Shell::the().run_script(file_path);
}

int main(int argc, char *argv[])
{
	Shell::the().set("PS1", "Shell $ ");
	Shell::the().set("SHELL", argv[0]);
	Shell::the().add_module<BuiltInsModule>();
	Shell::the().add_module<AliasModule>();

	if (argc <= 1)
	{
		run_if_exists(Shell::the().get_home() + "/.shellrc");

		Shell::the().add_module<AutoCompleteModule>();
		Shell::the().run();
		return 0;
	}

	auto script_path = argv[1];
	Shell::the().run_script(script_path);
	return 0;
}

