#include "dumpast.h"
#include <stdio.h>
#include <assert.h>

static void print_indent(int indent)
{
    for (int j = 0; j < indent; j++)
        printf("  ");
}

static void dump_value(Value *value, int indent)
{
    print_indent(indent);
    switch (value->type)
    {
        case VALUE_TYPE_INT:
            printf ("int %i\n", value->i);
            break;
        case VALUE_TYPE_FLOAT:
            printf ("float %f\n", value->f);
            break;
        case VALUE_TYPE_VARIABLE:
            printf ("variable %s\n", lexer_printable_token_data(&value->v));
            break;
        default:
            assert (0);
    }
}

static void dump_expression(Expression *expression, int indent)
{
    print_indent(indent);
    switch (expression->type)
    {
        case EXPRESSION_TYPE_VALUE:
            printf("Value:\n");
            dump_value(&expression->value, indent + 1);
            break;
        case EXPRESSION_TYPE_ADD:
            printf("Add:\n");
            dump_expression(expression->left, indent + 1);
            dump_expression(expression->right, indent + 1);
            break;
        default:
            assert (0);
    }
}

static void dump_scope(Scope *scope, int indent)
{
    for (int i = 0; i < scope->statement_count; i++)
    {
        Statement *statement = &scope->statements[i];
        print_indent(indent);

        switch (statement->type)
        {
            case STATEMENT_TYPE_DECLARATION:
                printf("Dec: %s: ", lexer_printable_token_data(&statement->name));
                printf("%s\n", lexer_printable_token_data(&statement->data_type.name));
                if (statement->expression)
                    dump_expression(statement->expression, indent + 1);
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
