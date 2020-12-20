#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#include "../lexer/lexer.h"

#define MAX_ARRAY_DEPTH 80

struct SymbolTable;

enum DataTypeFlags
{
    DATA_TYPE_CONST     = 1 << 0,
    DATA_TYPE_PRIMITIVE = 1 << 1,
    DATA_TYPE_STRUCT    = 1 << 2,
    DATA_TYPE_UNSIGNED  = 1 << 3,
    DATA_TYPE_ENUM      = 1 << 4,
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
    int pointer_count;
    int array_count;

    enum DataTypeFlags flags;
    enum Primitive primitive;
    struct SymbolTable *members;
    int array_sizes[MAX_ARRAY_DEPTH];
} DataType;

DataType dt_void();
DataType dt_int();
DataType dt_float();
DataType dt_double();
DataType dt_char();
DataType dt_const_char_pointer();

int data_type_equals(DataType *lhs, DataType *rhs);
int data_type_size(DataType *data_type);

DataType parse_data_type(struct SymbolTable *table);

#endif // DATA_TYPE_H

