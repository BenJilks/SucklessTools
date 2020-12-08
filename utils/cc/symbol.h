#ifndef SYMBOL_H
#define SYMBOL_H

#include "lexer.h"
#include "datatype.h"

enum SymbolFlags
{
    SYMBOL_NULL     = 1 << 0,
    SYMBOL_LOCAL    = 1 << 1,
    SYMBOL_GLOBAL   = 1 << 2,
    SYMBOL_FUNCTION = 1 << 3,
    SYMBOL_ARGUMENT = 1 << 4,
    SYMBOL_MEMBER   = 1 << 5,
    SYMBOL_ENUM     = 1 << 7,
};

typedef struct Symbol
{
    Token name;
    DataType data_type;
    int flags;
    int location;

    struct Symbol *params;
    int param_count;
    int is_variadic;
} Symbol;

typedef struct TypeDef
{
    Token name;
    DataType type;
} TypeDef;

typedef struct SymbolTable
{
    struct SymbolTable *parent;

    Symbol **symbols;
    int symbol_count;

    TypeDef *type_defs;
    int type_def_count;
} SymbolTable;

SymbolTable *symbol_table_new(SymbolTable *parent);

void symbol_table_add(SymbolTable *table, Symbol symbol);
void symbol_table_define_type(SymbolTable *table, Token name, DataType type);

Symbol *symbol_table_lookup(SymbolTable *table, Token *name);
DataType *symbol_table_lookup_type(SymbolTable *table, Token *name);
int symbol_table_size(SymbolTable *table);
int symbol_size(Symbol *symbol);

void free_symbol_table(SymbolTable *table);

#endif // SYMBOL_H

