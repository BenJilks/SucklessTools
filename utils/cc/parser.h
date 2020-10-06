#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symbol.h"
#include "expression.h"

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

typedef struct Struct
{
    Token name;
    SymbolTable *members;
} Struct;

typedef struct Unit
{
    Function *functions;
    int function_count;

    Struct *structs;
    int struct_count;

    SymbolTable *global_table;
} Unit;

typedef struct Buffer
{
    void *memory;
    int unit_size;

    int *count;
    int buffer;
} Buffer;
Buffer make_buffer(void **memory, int *count, int unit_size);
void append_buffer(Buffer*, void *item);

void match(enum TokenType, const char *name);
Unit *parse();
void free_unit(Unit *unit);

DataType dt_void();
DataType dt_int();
DataType dt_float();
DataType dt_double();
DataType dt_char();
DataType dt_const_char_pointer();

#endif // PARSER_H
