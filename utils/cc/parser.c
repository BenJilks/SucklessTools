#include <assert.h>
#include "parser.h"
#include "lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

static void match(enum TokenType type)
{
    Token token = lexer_consume(type);
    assert (type != TOKEN_TYPE_NONE && token.type != TOKEN_TYPE_NONE);
}

static DataType parse_data_type()
{
    DataType data_type;
    data_type.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
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

static void parse_params(Symbol *symbol)
{
    Buffer params_buffer = make_buffer((void**)&symbol->params, &symbol->param_count, sizeof(Symbol));

    match(TOKEN_TYPE_OPEN_BRACKET);
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_BRACKET)
    {
        Symbol param;
        param.data_type = parse_data_type();
        param.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
        param.params = NULL;
        param.param_count = 0;
        append_buffer(&params_buffer, &param);

        if (lexer_peek(0).type != TOKEN_TYPE_COMMA)
            break;
        match(TOKEN_TYPE_COMMA);
    }
    match(TOKEN_TYPE_CLOSE_BRACKET);
}

static void parse_function_body(Function *function)
{
    match(TOKEN_TYPE_OPEN_SQUIGGLY);
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_SQUIGGLY)
    {
        // TODO: Parse statements
    }
    match(TOKEN_TYPE_CLOSE_SQUIGGLY);
}

static void parse_function(Unit *unit)
{
    // Function definition
    Symbol func_symbol;
    func_symbol.data_type = parse_data_type();
    func_symbol.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    parse_params(&func_symbol);

    // Register symbol
    symbol_table_add(&unit->global_table, func_symbol);

    // Optional function body
    Function function;
    function.func_symbol = func_symbol;
    if (lexer_peek(0).type == TOKEN_TYPE_OPEN_SQUIGGLY)
        parse_function_body(&function);
    else
        match(TOKEN_TYPE_SEMI);

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
    unit.global_table = symbol_table_new(NULL);

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

void free_unit(Unit *unit)
{
    if (unit->functions)
        free(unit->functions);

    free_symbol_table(&unit->global_table);
}
