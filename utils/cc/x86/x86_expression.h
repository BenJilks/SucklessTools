#ifndef X86_EXPRESSION_H
#define X86_EXPRESSION_H

#include "../parser/parser.h"
#include "x86_asm.h"

#define INST(...) x86_code_add_instruction(code, x86(code, __VA_ARGS__))

enum ExpressionMode
{
    EXPRESSION_MODE_LHS,
    EXPRESSION_MODE_RHS,
};

typedef struct X86Value
{
    DataType data_type;
} X86Value;

X86Value compile_cast(X86Code *code, X86Value *value, DataType *data_type);
X86Value compile_expression(X86Code *code, Expression *expression, enum ExpressionMode mode);
void compile_assign_variable(X86Code *code, Symbol *variable, X86Value *value);

#endif // X86_EXPRESSION_H
