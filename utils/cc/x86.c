#include "x86.h"
#include <assert.h>
#include <stdio.h>

#define INST(...) x86_code_add_instruction(code, x86(code, __VA_ARGS__))

static void compile_string(X86Code *code, Value *value)
{
    int string_id = x86_code_add_string_data(code, lexer_printable_token_data(&value->s));
    char label[80];
    sprintf(label, "str%i", string_id);
    INST(X86_OP_CODE_PUSH_LABEL, label);
}

static void compile_value(X86Code *code, Value *value)
{
    switch (value->type)
    {
        case VALUE_TYPE_INT:
            INST(X86_OP_CODE_PUSH_IMM32, value->i);
            break;
        case VALUE_TYPE_FLOAT:
            INST(X86_OP_CODE_PUSH_IMM32, value->f);
            break;
        case VALUE_TYPE_STRING:
            compile_string(code, value);
            break;
        case VALUE_TYPE_VARIABLE:
            if (value->v->flags & SYMBOL_LOCAL)
                INST(X86_OP_CODE_PUSH_MEM_REG_OFF, X86_REG_EBP, -4 - value->v->location);
            else if (value->v->flags & SYMBOL_ARGUMENT)
                INST(X86_OP_CODE_PUSH_MEM_REG_OFF, X86_REG_EBP, 8 + value->v->location);
            else
                assert (0);
            break;
    }
}

static void compile_expression(X86Code *code, Expression *expression);
static void compile_fuction_call(X86Code *code, Expression *expression)
{
    Token func = expression->left->value.v->name;
    for (int i = expression->argument_length - 1; i >= 0; i--)
        compile_expression(code, expression->arguments[i]);

    INST(X86_OP_CODE_CALL_LABEL, lexer_printable_token_data(&func));
    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, expression->argument_length * 4);
}

static void compile_expression(X86Code *code, Expression *expression)
{
    switch (expression->type)
    {
        case EXPRESSION_TYPE_VALUE:
            compile_value(code, &expression->value);
            break;
        case EXPRESSION_TYPE_ADD:
            compile_expression(code, expression->left);
            compile_expression(code, expression->right);
            INST(X86_OP_CODE_POP_REG, X86_REG_EBX);
            INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
            INST(X86_OP_CODE_ADD_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            break;
        case EXPRESSION_TYPE_FUNCTION_CALL:
            compile_fuction_call(code, expression);
            break;
    }
}

static void compile_scope(X86Code *code, Scope *scope)
{
    for (int i = 0; i < scope->statement_count; i++)
    {
        Statement *statement = &scope->statements[i];
        switch (statement->type)
        {
            case STATEMENT_TYPE_DECLARATION:
                if (statement->expression)
                {
                    compile_expression(code, statement->expression);
                    INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
                    INST(X86_OP_CODE_MOV_MEM_REG_OFF_REG, X86_REG_EBP, -4 - statement->symbol.location, X86_REG_EAX);
                }
                break;
            case STATEMENT_TYPE_EXPRESSION:
                compile_expression(code, statement->expression);
                break;
            default:
                break;
        }
    }
}

static void compile_function(X86Code *code, Function *function)
{
    if (!function->body)
    {
        x86_code_add_external(code, lexer_printable_token_data(&function->func_symbol.name));
        return;
    }

    x86_code_add_label(code, lexer_printable_token_data(&function->func_symbol.name));
    INST(X86_OP_CODE_PUSH_REG, X86_REG_EBP);
    INST(X86_OP_CODE_MOV_REG_REG, X86_REG_EBP, X86_REG_ESP);
    INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, function->stack_size);
    x86_code_add_blank(code);
    compile_scope(code, function->body);

    x86_code_add_blank(code);
    INST(X86_OP_CODE_MOV_REG_REG, X86_REG_ESP, X86_REG_EBP);
    INST(X86_OP_CODE_POP_REG, X86_REG_EBP);
    INST(X86_OP_CODE_RET);
    x86_code_add_blank(code);
}

X86Code x86_compile_unit(Unit *unit)
{
    X86Code code = x86_code_new();
    for (int i = 0; i < unit->function_count; i++)
        compile_function(&code, &unit->functions[i]);

    return code;
}
