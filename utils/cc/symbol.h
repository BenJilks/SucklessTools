#ifndef SYMBOL_H
#define SYMBOL_H

#include "lexer.h"

enum DataTypeFlags
{
    DATA_TYPE_CONST = 1 >> 0,
};

typedef struct DataType
{
    Token name;

    enum DataTypeFlags flags;
    int pointer_count;
} DataType;

enum SymbolFlags
{
    SYMBOL_NULL     = 1 << 0,
    SYMBOL_LOCAL    = 1 << 1,
    SYMBOL_GLOBAL   = 1 << 2,
    SYMBOL_FUNCTION = 1 << 3,
    SYMBOL_ARGUMENT = 1 << 4,
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

typedef struct SymbolTable
{
    struct SymbolTable *parent;
    Symbol **symbols;
    int symbol_count;
} SymbolTable;

SymbolTable *symbol_table_new(SymbolTable *parent);
void symbol_table_add(SymbolTable *table, Symbol symbol);
Symbol *symbol_table_lookup(SymbolTable *table, Token *name);
void free_symbol_table(SymbolTable *table);

#endif // SYMBOL_H
