#include "x86.h"
#include "x86_expression.h"
#include "dumpast.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void compile_scope(X86Code *code, Scope *scope);

static void compile_function_return(X86Code *code)
{
    COMMENT_CODE(code, "Return");
    INST(X86_OP_CODE_MOV_REG_REG, X86_REG_ESP, X86_REG_EBP);
    INST(X86_OP_CODE_POP_REG, X86_REG_EBP);
    INST(X86_OP_CODE_RET);
}

static void compile_decleration(X86Code *code, Statement *statement)
{
    if (statement->expression)
    {
        X86Value value = compile_expression(code, statement->expression, EXPRESSION_MODE_RHS);
        compile_assign_variable(code, &statement->symbol, &value);
    }

    if (statement->next)
        compile_decleration(code, statement->next);
}

static void compile_if(X86Code *code, Statement *statement)
{
    char end_label[80];
    sprintf(end_label, "if%x", rand());

    X86Value value = compile_expression(code, statement->expression, EXPRESSION_MODE_RHS);
    DataType type = dt_int();
    compile_cast(code, &value, &type);

    INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
    INST(X86_OP_CODE_CMP_REG_IMM32, X86_REG_EAX, 0);
    INST(X86_OP_CODE_JUMP_LABEL_IF_ZERO, end_label);
    compile_scope(code, statement->sub_scope);
    x86_code_add_label(code, end_label);
}

static void compile_while(X86Code *code, Statement *statement)
{
    char end_label[80], start_label[80];
    sprintf(end_label, "while%x", rand());
    sprintf(start_label, "while%x", rand());

    x86_code_add_label(code, start_label);
    X86Value value = compile_expression(code, statement->expression, EXPRESSION_MODE_RHS);
    DataType type = dt_int();
    compile_cast(code, &value, &type);

    INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
    INST(X86_OP_CODE_CMP_REG_IMM32, X86_REG_EAX, 0);
    INST(X86_OP_CODE_JUMP_LABEL_IF_ZERO, end_label);
    compile_scope(code, statement->sub_scope);
    INST(X86_OP_CODE_JUMP_LABEL, start_label);
    x86_code_add_label(code, end_label);
}

static void compile_expression_statement(X86Code *code, Statement *statement)
{
    X86Value value = compile_expression(code, statement->expression, EXPRESSION_MODE_RHS);

    COMMENT_CODE(code, "Pop unused value");
    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, value.data_type.size);
}

static void compile_scope(X86Code *code, Scope *scope)
{
    for (int i = 0; i < scope->statement_count; i++)
    {
        Statement *statement = &scope->statements[i];
        switch (statement->type)
        {
            case STATEMENT_TYPE_DECLARATION:
                compile_decleration(code, statement);
                break;
            case STATEMENT_TYPE_EXPRESSION:
                compile_expression_statement(code, statement);
                break;
            case STATEMENT_TYPE_RETURN:
                compile_expression(code, statement->expression, EXPRESSION_MODE_RHS);
                INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
                compile_function_return(code);
                break;
            case STATEMENT_TYPE_IF:
                compile_if(code, statement);
                break;
            case STATEMENT_TYPE_WHILE:
                compile_while(code, statement);
                break;
            default:
                assert (0);
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
    compile_function_return(code);
    x86_code_add_blank(code);
}

X86Code x86_compile_unit(Unit *unit)
{
    X86Code code = x86_code_new();
    for (int i = 0; i < unit->function_count; i++)
        compile_function(&code, &unit->functions[i]);

    return code;
}
