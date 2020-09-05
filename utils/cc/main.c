#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

int main()
{
    lexer_open_file("test.txt");

    Unit unit = parse();
    printf("Function Count: %i\n", unit.function_count);
    for (int i = 0; i < unit.function_count; i++)
    {
        printf("  Function: %s\n", lexer_printable_token_data(&unit.functions[i].func_symbol.name));
        for (int j = 0; j < unit.functions[i].func_symbol.param_count; j++)
            printf("    Param: %s\n", lexer_printable_token_data(&unit.functions[i].func_symbol.params[j].name));
    }

    free_unit(&unit);
    lexer_close();
    return 0;
}
