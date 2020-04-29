#include "shell.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
	Shell::the().set("PS1", "Shell $ ");
	Shell::the().set("SHELL", argv[0]);
	Shell::the().run();

	return 0;
}
