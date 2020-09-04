#include <assert.h>
#include "parser.h"
#include "lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

static void match(enum TokenType type)
{
    Token token = lexer_consume(type);
    assert (type != TOKEN_TYPE_NONE);

    free(token.data);
}

static DataType parse_data_type()
{
    DataType data_type;
    data_type.name = lexer_consume(TOKEN_TYPE_IDENTIFIER).data;
    return data_type;
}

typedef struct Buffer
{
    void *memory;
    int unit_size;

    int *count;
    int buffer;
} Buffer;

static Buffer make_buffer(void **memory, int *count, int unit_size)
{
    Buffer buffer;
    buffer.memory = malloc(80 * unit_size);
    buffer.unit_size = unit_size;
    buffer.count = count;
    buffer.buffer = 80;

    *count = 0;
    *memory = buffer.memory;
    return buffer;
}

static void append_buffer(Buffer *buffer, void *item)
{
    if (*buffer->count >= buffer->buffer)
    {
        buffer->buffer += 80;
        buffer->memory = realloc(buffer->memory, buffer->buffer * buffer->unit_size);
    }

    memcpy(buffer->memory + (*buffer->count * buffer->unit_size), item, buffer->unit_size);
    *buffer->count += 1;
}

static void parse_params(Function *function)
{
    Buffer params_buffer = make_buffer((void**)&function->params, &function->param_count, sizeof(Param));

    match(TOKEN_TYPE_OPEN_BRACKET);
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_BRACKET)
    {
        Param param;
        param.data_type = parse_data_type();
        param.name = lexer_consume(TOKEN_TYPE_IDENTIFIER).data;
        append_buffer(&params_buffer, &param);

        if (lexer_peek(0).type != TOKEN_TYPE_COMMA)
            break;
        lexer_consume(TOKEN_TYPE_COMMA);
    }
    match(TOKEN_TYPE_CLOSE_BRACKET);
}

static void parse_function(Unit *unit)
{
    Function function;
    function.data_type = parse_data_type();
    function.name = lexer_consume(TOKEN_TYPE_IDENTIFIER).data;
    parse_params(&function);

    if (!unit->functions)
        unit->functions = malloc(sizeof(Function));
    else
        unit->functions = realloc(unit->functions, (unit->function_count + 1) * sizeof(Function));
    unit->functions[unit->function_count++] = function;
}

Unit parse()
{
    Unit unit;
    unit.functions = NULL;
    unit.function_count = 0;

    for (;;)
    {
        Token token = lexer_peek(0);
        if (token.type == TOKEN_TYPE_NONE)
            break;

        switch (token.type)
        {
            case TOKEN_TYPE_IDENTIFIER:
                parse_function(&unit);
                break;

            default:
                assert (0);
        }
    }

    return unit;
}

static void free_data_type(DataType *data_type)
{
    free(data_type->name);
}

static void free_function(Function *function)
{
    free_data_type(&function->data_type);
    free(function->name);
}

void free_unit(Unit *unit)
{
    if (unit->functions)
    {
        for (int i = 0; i < unit->function_count; i++)
            free_function(&unit->functions[i]);
        free(unit->functions);
    }
}
