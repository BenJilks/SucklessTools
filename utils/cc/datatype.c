#include "datatype.h"
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

#define PRIMITIVE_DATA_TYPE(name, primitive) \
    DataType dt_##name() \
    { \
        return (DataType){ { #name, sizeof(#name), TOKEN_TYPE_IDENTIFIER, { "null", 0, 0, 0 } }, sizeof(name), DATA_TYPE_PRIMITIVE, primitive, NULL, 0 }; \
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

int data_type_size(DataType *data_type)
{
    if (data_type->pointer_count > 0)
        return 4; // NOTE: 32bit only
    return data_type->size;
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
    DataType data_type;
    data_type.flags = 0;
    data_type.pointer_count = 0;
    data_type.members = NULL;
    if (lexer_peek(0).type == TOKEN_TYPE_CONST)
    {
        match(TOKEN_TYPE_CONST, "const");
        data_type.flags |= DATA_TYPE_CONST;
    }

    data_type.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    data_type.primitive = primitive_from_name(&data_type.name);
    if (data_type.primitive == PRIMITIVE_NONE)
        ERROR(&data_type.name, "Expected datatype, got '%s' instead", lexer_printable_token_data(&data_type.name));

    data_type.size = size_from_primitive(data_type.primitive);
    data_type.flags |= DATA_TYPE_PRIMITIVE;
    parse_type_pointers(&data_type);
    return data_type;
}

static Struct *find_struct(Unit *unit, Token *name)
{
    for (int i = 0; i < unit->struct_count; i++)
    {
        if (lexer_compair_token_token(&unit->structs[i].name, name))
            return &unit->structs[i];
    }

    return NULL;
}

static DataType parse_struct_type(Unit *unit)
{
    DataType data_type;
    data_type.flags = DATA_TYPE_STRUCT;
    data_type.pointer_count = 0;
    data_type.primitive = PRIMITIVE_NONE;

    match(TOKEN_TYPE_STRUCT, "struct");
    Token name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    Struct *struct_ = find_struct(unit, &name);
    data_type.name = name;

    if (struct_ == NULL)
    {
        ERROR(&name, "No struct with the name '%s' found\n",
            lexer_printable_token_data(&name));
    }
    else
    {
        data_type.size = symbol_table_size(struct_->members);
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

DataType parse_data_type(Unit *unit, SymbolTable *table)
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
        return parse_struct_type(unit);
    else if (name.type == TOKEN_TYPE_UNSIGNED)
        return parse_unsigned_type();

    // Otherwise, parse normally
    return parse_primitive();
}
