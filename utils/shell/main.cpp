#include "shell.hpp"
#include <iostream>

#include "modules/autocomplete.hpp"
#include "modules/builtins.hpp"

int main(int argc, char *argv[])
{
	Shell::the().set("PS1", "Shell $ ");
	Shell::the().set("SHELL", argv[0]);
	Shell::the().add_module<BuiltInsModule>();

	if (argc <= 1)
	{
		Shell::the().add_module<AutoCompleteModule>();
		Shell::the().run();
		return 0;
	}

	auto script_path = argv[1];
	Shell::the().run_script(script_path);
	return 0;
}
