#include "symbol.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

SymbolTable *symbol_table_new(SymbolTable *parent)
{
    SymbolTable *table = malloc(sizeof(SymbolTable));
    table->parent = parent;
    table->symbols = NULL;
    table->symbol_count = 0;
    table->type_defs = NULL;
    table->type_def_count = 0;
    return table;
}

void symbol_table_add(SymbolTable *table, Symbol symbol)
{
    // TODO: Don't allocate each time
    if (!table->symbols)
        table->symbols = malloc(sizeof(Symbol*));
    else
        table->symbols = realloc(table->symbols, (table->symbol_count + 1) * sizeof(Symbol*));

    table->symbols[table->symbol_count] = malloc(sizeof(Symbol));
    memcpy(table->symbols[table->symbol_count], &symbol, sizeof(Symbol));
    table->symbol_count += 1;
}

void symbol_table_define_type(SymbolTable *table, Token name, DataType type)
{
    // TODO: Don't allocate each time
    if (!table->type_defs)
        table->type_defs = malloc(sizeof(TypeDef));
    else
        table->type_defs = realloc(table->type_defs, (table->type_def_count + 1) * sizeof(TypeDef));

    TypeDef *type_def = &table->type_defs[table->type_def_count++];
    type_def->name = name;
    type_def->type = type;
}

Symbol *symbol_table_lookup(SymbolTable *table, Token *name)
{
    for (int i = table->symbol_count - 1; i >= 0; i--)
    {
        Symbol *symbol = table->symbols[i];
        if (lexer_compair_token_token(&symbol->name, name))
            return symbol;
    }

    if (table->parent)
        return symbol_table_lookup(table->parent, name);

    return NULL;
}

DataType *symbol_table_lookup_type(SymbolTable *table, Token *name)
{
    for (int i = 0; i < table->type_def_count; i++)
    {
        TypeDef *type_def = &table->type_defs[i];
        if (lexer_compair_token_token(&type_def->name, name))
            return &type_def->type;
    }

    if (table->parent)
        return symbol_table_lookup_type(table->parent, name);

    return NULL;
}

int symbol_table_size(SymbolTable *table)
{
    int total_size = 0;
    for (int i = 0; i < table->symbol_count; i++)
        total_size += data_type_size(&table->symbols[i]->data_type);

    if (table->parent)
        total_size += symbol_table_size(table->parent);

    return total_size;
}

int symbol_size(Symbol *symbol)
{
    return data_type_size(&symbol->data_type);
}

void free_symbol_table(SymbolTable *table)
{
    if (table->symbols)
    {
        for (int i = 0; i < table->symbol_count; i++)
        {
            if (table->symbols[i]->params)
                free(table->symbols[i]->params);
            free(table->symbols[i]);
        }
        free(table->symbols);
        table->symbols = NULL;
    }
    if (table->type_defs)
    {
        free(table->type_defs);
        table->type_defs = NULL;
    }
}

