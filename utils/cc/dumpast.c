#include "dumpast.h"
#include "parser/unit.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void print_indent(int indent)
{
    for (int j = 0; j < indent; j++)
        fprintf(stderr, "  ");
}

static void dump_value(Value *value, int indent)
{
    print_indent(indent);
    switch (value->type)
    {
        case VALUE_TYPE_INT:
            fprintf(stderr, "int %i\n", value->i);
            break;
        case VALUE_TYPE_FLOAT:
            fprintf(stderr, "float %f\n", value->f);
            break;
        case VALUE_TYPE_STRING:
            fprintf(stderr, "string \"%s\"\n", lexer_printable_token_data(&value->s));
            break;
        case VALUE_TYPE_CHAR:
            fprintf(stderr, "char '%s'\n", lexer_printable_token_data(&value->s));
            break;
        case VALUE_TYPE_VARIABLE:
            if (value->v == NULL)
                fprintf(stderr, "variable %s\n", "!! Unkown !!");
            else
                fprintf(stderr, "variable %s\n", lexer_printable_token_data(&value->v->name));
            break;
        default:
            assert (0);
    }
}

static void dump_expression(Expression *expression, int indent)
{
#define PRINT_DATA_TYPE \
    fprintf(stderr, " -> %s\n", printable_data_type(&expression->data_type));

    print_indent(indent);
    switch (expression->type)
    {
        case EXPRESSION_TYPE_VALUE:
            fprintf(stderr, "Value:");
            PRINT_DATA_TYPE; 
            dump_value(&expression->value, indent + 1);
            break;
        case EXPRESSION_TYPE_REF:
            fprintf(stderr, "Ref:");
            PRINT_DATA_TYPE; 
            dump_expression(expression->left, indent + 1);
            break;
        case EXPRESSION_TYPE_INVERT:
            fprintf(stderr, "Invert:");
            PRINT_DATA_TYPE; 
            dump_expression(expression->left, indent + 1);
            break;
        case EXPRESSION_TYPE_CAST:
            fprintf(stderr, "Cast:");
            PRINT_DATA_TYPE; 
            dump_expression(expression->left, indent + 1);
            print_indent(indent + 1);
            fprintf(stderr, "To '%s'\n", printable_data_type(&expression->data_type));
            break;
        case EXPRESSION_TYPE_FUNCTION_CALL:
            fprintf(stderr, "Call:");
            PRINT_DATA_TYPE; 
            dump_expression(expression->left, indent + 1);
            print_indent(indent);
            fprintf(stderr, "Arguments:\n");
            for (int i = 0; i < expression->argument_length; i++)
                dump_expression(expression->arguments[i], indent + 1);
            break;
        default:
            fprintf(stderr, "%s:", expression_type_name(expression->type));
            PRINT_DATA_TYPE; 
            dump_expression(expression->left, indent + 1);
            dump_expression(expression->right, indent + 1);
            break;
    }
}

char *printable_data_type(DataType *data_type)
{
    static char buffer[1024];
    int buffer_pointer = 0;
    if (data_type->flags & DATA_TYPE_CONST)
    {
        sprintf(buffer, "const ");
        buffer_pointer += 6;
    }
    if (data_type->flags & DATA_TYPE_UNSIGNED)
    {
        sprintf(buffer, "unsigned ");
        buffer_pointer += 9;
    }
    sprintf(buffer + buffer_pointer, "%s", lexer_printable_token_data(&data_type->name));
    buffer_pointer = strlen(buffer);

    for (int i = 0; i < data_type->pointer_count; i++)
    {
        sprintf(buffer + buffer_pointer, "*");
        buffer_pointer += 1;
    }

    if (data_type->array_count > 0)
    {
        sprintf(buffer + (buffer_pointer++), " ");
        for (int i = 0; i < data_type->array_count; i++)
        {
            sprintf(buffer + buffer_pointer, "[%i]", data_type->array_sizes[i]);
            buffer_pointer = strlen(buffer);
        }
    }
    return buffer;
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
                fprintf(stderr, "Dec: %s: ", lexer_printable_token_data(&statement->symbol.name));
                fprintf(stderr, "%s\n", printable_data_type(&statement->symbol.data_type));
                if (statement->expression)
                    dump_expression(statement->expression, indent + 1);
                break;

            case STATEMENT_TYPE_EXPRESSION:
                fprintf(stderr, "Expression:\n");
                dump_expression(statement->expression, indent + 1);
                break;

            case STATEMENT_TYPE_RETURN:
                fprintf(stderr, "Return:\n");
                dump_expression(statement->expression, indent + 1);
                break;

            case STATEMENT_TYPE_IF:
                fprintf(stderr, "If:\n");
                dump_expression(statement->expression, indent + 1);
                print_indent(indent);
                fprintf(stderr, "Body:\n");
                dump_scope(statement->sub_scope, indent + 1);
                break;

            case STATEMENT_TYPE_WHILE:
                fprintf(stderr, "While:\n");
                dump_expression(statement->expression, indent + 1);
                print_indent(indent);
                fprintf(stderr, "Body:\n");
                dump_scope(statement->sub_scope, indent + 1);
                break;

            default:
                assert (0);
        }
    }
}

static void dump_function(Function *function)
{
    fprintf(stderr, "  Function %s(", lexer_printable_token_data(&function->func_symbol.name));
    for (int i = 0; i < function->func_symbol.param_count; i++)
    {
        Symbol *param = &function->func_symbol.params[i];
        fprintf(stderr, "%s ", printable_data_type(&param->data_type));
        fprintf(stderr, "%s", lexer_printable_token_data(&param->name));
        if (i != function->func_symbol.param_count - 1)
            fprintf(stderr, ", ");
    }
    fprintf(stderr, ") -> %s\n", printable_data_type(&function->func_symbol.data_type));
    if (function->body)
        dump_scope(function->body, 2);
}

static void dump_struct(Struct *struct_)
{
    fprintf(stderr, "  Struct %s\n", lexer_printable_token_data(&struct_->name));
    for (int i = 0; i < struct_->members->symbol_count; i++)
    {
        Symbol *member = struct_->members->symbols[i];
        fprintf(stderr, "    %s: ", lexer_printable_token_data(&member->name));
        fprintf(stderr, "%s\n", printable_data_type(&member->data_type));
    }
}

void dump_unit()
{
    fprintf(stderr, "Unit:\n");
    for (int i = 0; i < struct_count; i++)
        dump_struct(&structs[i]);
    for (int i = 0; i < function_count; i++)
        dump_function(&functions[i]);
}
