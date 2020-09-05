#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef struct DataType
{
    Token name;
} DataType;

typedef struct Param
{
    Token name;
    DataType data_type;
} Param;

typedef struct Function
{
    Token name;
    DataType data_type;

    Param *params;
    int param_count;
} Function;

typedef struct Unit
{
    Function *functions;
    int function_count;
} Unit;

Unit parse();
void free_unit(Unit *unit);

#endif // PARSER_H
