#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symbol.h"

typedef struct Function
{
    Symbol func_symbol;
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
