#ifndef SYMBOL_H
#define SYMBOL_H

#include "lexer.h"

typedef struct DataType
{
    Token name;
} DataType;

enum SymbolFlags
{
    SYMBOL_NULL,
    SYMBOL_LOCAL,
    SYMBOL_GLOBAL,
    SYMBOL_FUNCTION
};

typedef struct Symbol
{
    Token name;
    DataType data_type;
    int flags;

    struct Symbol *params;
    int param_count;
} Symbol;

typedef struct SymbolTable
{
    struct SymbolTable *parent;
    Symbol *symbols;
    int symbol_count;
} SymbolTable;

SymbolTable symbol_table_new(SymbolTable *parent);
void symbol_table_add(SymbolTable *table, Symbol symbol);
Symbol *symbol_table_lookup(SymbolTable *table, const char *name);
void free_symbol_table(SymbolTable *table);

#endif // SYMBOL_H
