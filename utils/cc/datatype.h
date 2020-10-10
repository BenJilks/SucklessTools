#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#include "lexer.h"

struct SymbolTable;
struct Unit;

enum DataTypeFlags
{
    DATA_TYPE_CONST     = 1 << 0,
    DATA_TYPE_PRIMITIVE = 1 << 1,
    DATA_TYPE_STRUCT    = 1 << 2,
    DATA_TYPE_UNSIGNED  = 1 << 3,
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

typedef struct DataType
{
    Token name;
    int size;

    enum DataTypeFlags flags;
    enum Primitive primitive;
    struct SymbolTable *members;
    int pointer_count;
} DataType;

DataType dt_void();
DataType dt_int();
DataType dt_float();
DataType dt_double();
DataType dt_char();
DataType dt_const_char_pointer();

int data_type_equals(DataType *lhs, DataType *rhs);
int data_type_size(DataType *data_type);
DataType parse_data_type(struct Unit *unit, struct SymbolTable *table);

#endif // DATA_TYPE_H
