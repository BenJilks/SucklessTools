#include "symbol.h"
#include <stdlib.h>
#include <string.h>

SymbolTable symbol_table_new(SymbolTable *parent)
{
    SymbolTable table;
    table.parent = parent;
    table.symbols = NULL;
    table.symbol_count = 0;
    return table;
}

void symbol_table_add(SymbolTable *table, Symbol symbol)
{
    // TODO: Don't allocate each time
    if (!table->symbols)
        table->symbols = malloc(sizeof(Symbol));
    else
        table->symbols = realloc(table->symbols, (table->symbol_count + 1) * sizeof(Symbol));

    table->symbols[table->symbol_count] = symbol;
    table->symbol_count += 1;
}

Symbol *symbol_table_lookup(SymbolTable *table, const char *name)
{
    for (int i = table->symbol_count - 1; i >= 0; i--)
    {
        Symbol *symbol = &table->symbols[i];
        if (lexer_compair_token_name(&symbol->name, name))
            return symbol;
    }

    if (table->parent)
        return symbol_table_lookup(table->parent, name);

    return NULL;
}

void free_symbol_table(SymbolTable *table)
{
    if (table->symbols)
    {
        for (int i = 0; i < table->symbol_count; i++)
        {
            if (table->symbols[i].params)
                free(table->symbols[i].params);
        }
        free(table->symbols);
    }
}
