#include "shell.hpp"

int main()
{
	Shell shell;
	shell.set("PS1", "Shell $ ");

	shell.run();

	return 0;
}
