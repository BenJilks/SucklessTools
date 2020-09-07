#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "dumpast.h"
#include "x86.h"

int main()
{
    lexer_open_file("test.txt");

    // Parse
    Unit *unit = parse();

    // Compile
    X86Code code = x86_compile_unit(unit);
    x86_dump(&code);

    // Clean up
    free_x86_code(&code);
    free_unit(unit);
    free(unit);

    lexer_close();
    return 0;
}
