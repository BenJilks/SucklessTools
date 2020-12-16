#include "datatype.h"
#include "parser.h"
#include "unit.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define PRIMITIVE_DATA_TYPE(_name, _primitive) \
    DataType dt_##_name() \
    { \
        DataType type = new_data_type(); \
        type.name = (Token) { #_name, sizeof(#_name) - 1, \
            TOKEN_TYPE_IDENTIFIER, { "null", 0, 0, 0 } }; \
         \
        type.flags = DATA_TYPE_PRIMITIVE; \
        type.primitive = _primitive; \
        return type; \
    }

static DataType new_data_type()
{
    // Create a completely empty type
    DataType type;
    type.name = (Token) { "null", 4 };
    type.pointer_count = 0;
    type.array_count = 0;
    type.flags = 0;
    type.members = NULL;
    return type;
}

PRIMITIVE_DATA_TYPE(void, PRIMITIVE_VOID);
PRIMITIVE_DATA_TYPE(int, PRIMITIVE_INT);
PRIMITIVE_DATA_TYPE(float, PRIMITIVE_FLOAT);
PRIMITIVE_DATA_TYPE(double, PRIMITIVE_DOUBLE);
PRIMITIVE_DATA_TYPE(char, PRIMITIVE_CHAR);
DataType dt_const_char_pointer()
{
    DataType type = dt_char();
    type.pointer_count += 1;
    type.flags |= DATA_TYPE_CONST;
    return type;
}

int data_type_equals(DataType *lhs, DataType *rhs)
{
    if (lhs->flags != rhs->flags)
        return 0;
    if (lhs->pointer_count != rhs->pointer_count)
        return 0;
    if (!lexer_compair_token_token(&lhs->name, &rhs->name))
        return 0;
    return 1;
}

static int size_from_primitive(enum Primitive primitive)
{
    switch (primitive)
    {
        case PRIMITIVE_VOID:
            return 0;
        case PRIMITIVE_INT:
            return 4;
        case PRIMITIVE_FLOAT:
            return 4;
        case PRIMITIVE_DOUBLE:
            return 8;
        case PRIMITIVE_CHAR:
            return 1;
        default:
            return 0;
    }
}

int data_type_size(DataType *data_type)
{
    // TODO: This can be easily cached.

    // Find the base type size, ignoring arrays
    int base_size;
    if (data_type->pointer_count > 0)
        base_size = 4; // NOTE: 32bit only
    else if (data_type->flags & DATA_TYPE_PRIMITIVE)
        base_size = size_from_primitive(data_type->primitive);
    else if (data_type->flags & DATA_TYPE_STRUCT)
        base_size = symbol_table_size(data_type->members);
    else
        assert(0);

    // Calculate total size from arrays
    for (int i = 0; i < data_type->array_count; i++)
        base_size *= data_type->array_sizes[i];
    return base_size;
}

static void parse_type_pointers(DataType *data_type)
{
    while (lexer_peek(0).type == TOKEN_TYPE_STAR)
    {
        match(TOKEN_TYPE_STAR, "*");
        data_type->pointer_count += 1;
    }
}

static DataType parse_primitive()
{
    DataType data_type = new_data_type();
    if (lexer_peek(0).type == TOKEN_TYPE_CONST)
    {
        match(TOKEN_TYPE_CONST, "const");
        data_type.flags |= DATA_TYPE_CONST;
    }

    data_type.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    data_type.primitive = primitive_from_name(&data_type.name);
    if (data_type.primitive == PRIMITIVE_NONE)
    {
        ERROR(&data_type.name, "Expected datatype, got '%s' instead",
            lexer_printable_token_data(&data_type.name));
    }

    data_type.flags |= DATA_TYPE_PRIMITIVE;
    parse_type_pointers(&data_type);
    return data_type;
}

static DataType parse_struct_type()
{
    DataType data_type = new_data_type();
    data_type.flags = DATA_TYPE_STRUCT;
    data_type.primitive = PRIMITIVE_NONE;

    match(TOKEN_TYPE_STRUCT, "struct");
    Token name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    Struct *struct_ = unit_find_struct(&name);
    data_type.name = name;

    if (struct_ == NULL)
    {
        ERROR(&name, "No struct with the name '%s' found\n",
            lexer_printable_token_data(&name));
    }
    else
    {
        data_type.members = struct_->members;
    }

    parse_type_pointers(&data_type);
    return data_type;
}

static DataType parse_unsigned_type()
{
    match(TOKEN_TYPE_UNSIGNED, "unsigned");

    // The default base type is 'int'
    DataType data_type = dt_int();
    if (lexer_peek(0).type == TOKEN_TYPE_IDENTIFIER)
    {
        // Check if the next token contains a primitive type
        Token name = lexer_peek(0);
        enum Primitive primitive = primitive_from_name(&name);

        // If so, parse it and use it as the base type
        if (primitive != PRIMITIVE_NONE)
            data_type = parse_primitive();
    }

    data_type.flags |= DATA_TYPE_UNSIGNED;
    return data_type;
}

DataType parse_data_type(SymbolTable *table)
{
    // Check for typedefs
    Token name = lexer_peek(0);
    DataType *type_def = symbol_table_lookup_type(table, &name);
    if (type_def != NULL)
    {
        lexer_consume(TOKEN_TYPE_IDENTIFIER);
        parse_type_pointers(type_def);
        return *type_def;
    }

    if (name.type == TOKEN_TYPE_STRUCT)
        return parse_struct_type();
    else if (name.type == TOKEN_TYPE_UNSIGNED)
        return parse_unsigned_type();

    // Otherwise, parse normally
    return parse_primitive();
}

