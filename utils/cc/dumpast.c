#include "dumpast.h"
#include <stdio.h>

static void dump_function(Function *function)
{
    printf("  Function %s(", lexer_printable_token_data(&function->func_symbol.name));
    for (int i = 0; i < function->func_symbol.param_count; i++)
    {
        Symbol *param = &function->func_symbol.params[i];
        printf("%s ", lexer_printable_token_data(&param->data_type.name));
        printf("%s", lexer_printable_token_data(&param->name));
        if (i != function->func_symbol.param_count - 1)
            printf(", ");
    }
    printf(") -> %s\n", lexer_printable_token_data(&function->func_symbol.data_type.name));
}

void dump_unit(Unit *unit)
{
    printf("Unit:\n");
    for (int i = 0; i < unit->function_count; i++)
        dump_function(&unit->functions[i]);
}
