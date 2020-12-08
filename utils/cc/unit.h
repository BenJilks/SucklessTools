#ifndef UNIT_H
#define UNIT_H

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

typedef struct Enum
{
    Token name;
    SymbolTable *members;
} Enum;

extern Function *functions;
extern Struct *structs;
extern int function_count;
extern int struct_count;
extern SymbolTable *global_table;

void unit_create();
void unit_destroy();

void unit_add_function(Function*);
void unit_add_struct(Struct*);
void unit_add_enum(Enum*);

Struct *unit_find_struct(Token *name);

#endif // UNIT_H

