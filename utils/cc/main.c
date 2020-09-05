#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "dumpast.h"

int main()
{
    lexer_open_file("test.txt");

    Unit unit = parse();
    dump_unit(&unit);
    free_unit(&unit);

    lexer_close();
    return 0;
}
