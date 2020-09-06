#include "dumpast.h"
#include <stdio.h>
#include <assert.h>

static void dump_scope(Scope *scope, int indent)
{
    for (int i = 0; i < scope->statement_count; i++)
    {
        Statement *statement = &scope->statements[i];
        for (int j = 0; j < indent; j++)
            printf("  ");

        switch (statement->type)
        {
            case STATEMENT_TYPE_DECLARATION:
                printf("Dec: %s: ", lexer_printable_token_data(&statement->name));
                printf("%s\n", lexer_printable_token_data(&statement->data_type.name));
                break;

            default:
                assert (0);
        }
    }
}

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
    if (function->body)
        dump_scope(function->body, 2);
}

void dump_unit(Unit *unit)
{
    printf("Unit:\n");
    for (int i = 0; i < unit->function_count; i++)
        dump_function(&unit->functions[i]);
}
