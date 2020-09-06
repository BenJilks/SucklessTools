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

typedef struct Statement
{
    enum StatementType type;

    Token name;
    DataType data_type;
} Statement;

typedef struct Scope
{
    Statement *statements;
    int statement_count;

    SymbolTable table;
} Scope;

typedef struct Function
{
    Symbol func_symbol;
    Scope *body;
} Function;

typedef struct Unit
{
    Function *functions;
    int function_count;

    SymbolTable global_table;
} Unit;

Unit parse();
void free_unit(Unit *unit);

#endif // PARSER_H
