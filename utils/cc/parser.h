#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symbol.h"

#define ENUMERATE_STATEMENT_TYPE \
    __TYPE(DECLARATION) \
    __TYPE(EXPRESSION) \
    __TYPE(IF) \
    __TYPE(FOR) \
    __TYPE(WHILE) \
    __TYPE(RETURN)

enum StatementType
{
#define __TYPE(x) STATEMENT_TYPE_##x,
    ENUMERATE_STATEMENT_TYPE
#undef __TYPE
};

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

#define ENUMERATE_EXPRESSION_TYPE \
    __TYPE(VALUE) \
    __TYPE(ADD) \
    __TYPE(MUL) \
    __TYPE(SUB) \
    __TYPE(LESS_THAN) \
    __TYPE(REF) \
    __TYPE(FUNCTION_CALL)

enum ExpressionType
{
#define __TYPE(x) EXPRESSION_TYPE_##x,
    ENUMERATE_EXPRESSION_TYPE
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

struct Scope;
typedef struct Statement
{
    enum StatementType type;

    Symbol symbol;
    Expression *expression;

    // NOTE: Used for declaration lists
    struct Statement *next;
    struct Scope *sub_scope;
} Statement;

typedef struct Scope
{
    Statement *statements;
    int statement_count;

    SymbolTable *table;
} Scope;

typedef struct Function
{
    Symbol func_symbol;
    SymbolTable *table;
    Scope *body;
    int stack_size;
    int argument_size;
} Function;

typedef struct Unit
{
    Function *functions;
    int function_count;

    SymbolTable *global_table;
} Unit;

Unit *parse();
void free_unit(Unit *unit);

DataType dt_void();
DataType dt_int();
DataType dt_float();
DataType dt_double();
DataType dt_char();
DataType dt_const_char_pointer();

#endif // PARSER_H
