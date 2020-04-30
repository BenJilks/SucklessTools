#include "shell.hpp"
#include <iostream>

#include "modules/autocomplete.hpp"

int main(int argc, char *argv[])
{
	Shell::the().set("PS1", "Shell $ ");
	Shell::the().set("SHELL", argv[0]);

	Shell::the().add_module<AutoCompleteModule>();
	Shell::the().run();

	return 0;
}
