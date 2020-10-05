#ifndef SYMBOL_H
#define SYMBOL_H

#include "lexer.h"

enum DataTypeFlags
{
    DATA_TYPE_CONST     = 1 << 0,
    DATA_TYPE_PRIMITIVE = 1 << 1,
    DATA_TYPE_STRUCT    = 1 << 2,
};

enum Primitive
{
    PRIMITIVE_NONE,
    PRIMITIVE_VOID,
    PRIMITIVE_INT,
    PRIMITIVE_FLOAT,
    PRIMITIVE_DOUBLE,
    PRIMITIVE_CHAR,
};

struct SymbolTable;
typedef struct DataType
{
    Token name;
    int size;

    enum DataTypeFlags flags;
    enum Primitive primitive;
    struct SymbolTable *members;
    int pointer_count;
} DataType;

enum SymbolFlags
{
    SYMBOL_NULL     = 1 << 0,
    SYMBOL_LOCAL    = 1 << 1,
    SYMBOL_GLOBAL   = 1 << 2,
    SYMBOL_FUNCTION = 1 << 3,
    SYMBOL_ARGUMENT = 1 << 4,
    SYMBOL_MEMBER   = 1 << 5,
    SYMBOL_ARRAY    = 1 << 6,
};

typedef struct Symbol
{
    Token name;
    DataType data_type;
    int flags;
    int location;
    int array_count;

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
int data_type_equals(DataType *lhs, DataType *rhs);
int data_type_size(DataType *data_type);
int symbol_size(Symbol *symbol);

void free_symbol_table(SymbolTable *table);

#endif // SYMBOL_H
