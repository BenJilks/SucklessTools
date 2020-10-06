#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "symbol.h"

#define ENUMERATE_VALUE_TYPE \
    __TYPE(INT) \
    __TYPE(FLOAT) \
    __TYPE(STRING) \
    __TYPE(VARIABLE)

enum ValueType
{
#define __TYPE(x) VALUE_TYPE_##x,
    ENUMERATE_VALUE_TYPE
#undef __TYPE
};

typedef struct Value
{
    enum ValueType type;
    union
    {
        int i;
        float f;
        Symbol *v;
        Token s;
    };
} Value;

#define ENUMERATE_EXPRESSION_TYPE \
    __TYPE(VALUE) \
    __TYPE(ASSIGN) \
    __TYPE(ADD) \
    __TYPE(MUL) \
    __TYPE(SUB) \
    __TYPE(LESS_THAN) \
    __TYPE(REF) \
    __TYPE(DOT) \
    __TYPE(INDEX) \
    __TYPE(FUNCTION_CALL)

enum ExpressionType
{
#define __TYPE(x) EXPRESSION_TYPE_##x,
    ENUMERATE_EXPRESSION_TYPE
#undef __TYPE
};

typedef struct Expression
{
    enum ExpressionType type;
    DataType data_type;

    Value value;
    struct Expression *left;
    struct Expression *right;

    struct Expression **arguments;
    int argument_length;
} Expression;

Expression *parse_expression(SymbolTable *table);

#endif // EXPRESSION_H
