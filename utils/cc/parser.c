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
    data_type.flags = 0;
    data_type.pointer_count = 0;
    if (lexer_peek(0).type == TOKEN_TYPE_CONST)
    {
        match(TOKEN_TYPE_CONST);
        data_type.flags |= DATA_TYPE_CONST;
    }

    data_type.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    while (lexer_peek(0).type == TOKEN_TYPE_STAR)
    {
        match(TOKEN_TYPE_STAR);
        data_type.pointer_count += 1;
    }
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

static void add_statement_to_scope(Scope *scope, Statement statement)
{
    if (!scope->statements)
        scope->statements = malloc(sizeof(Statement));
    else
        scope->statements = realloc(scope->statements, (scope->statement_count + 1) * sizeof(Statement));

    scope->statements[scope->statement_count] = statement;
    scope->statement_count += 1;
}

static Expression *parse_expression();
static Expression *parse_function_call(Expression *left)
{
    Expression *call = malloc(sizeof(Expression));
    call->type = EXPRESSION_TYPE_FUNCTION_CALL;
    call->left = left;

    Buffer buffer = make_buffer((void**)&call->arguments, &call->argument_length, sizeof(Expression*));
    match(TOKEN_TYPE_OPEN_BRACKET);
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_BRACKET)
    {
        Expression *argument = parse_expression();
        append_buffer(&buffer, &argument);

        if (lexer_peek(0).type != TOKEN_TYPE_COMMA)
            break;
        match(TOKEN_TYPE_COMMA);
    }
    match(TOKEN_TYPE_CLOSE_BRACKET);

    return call;
}

static Expression *parse_term(SymbolTable *table)
{
    Expression *expression = malloc(sizeof(Expression));
    expression->type = EXPRESSION_TYPE_VALUE;

    Value *value = &expression->value;
    Token token = lexer_peek(0);
    switch (token.type)
    {
        case TOKEN_TYPE_INTEGER:
            value->type = VALUE_TYPE_INT;
            value->i = atoi(token.data);
            lexer_consume(TOKEN_TYPE_INTEGER);
            break;
        case TOKEN_TYPE_IDENTIFIER:
            value->type = VALUE_TYPE_VARIABLE;
            value->v = symbol_table_lookup(table, &token);
            lexer_consume(TOKEN_TYPE_IDENTIFIER);
            break;
        default:
            assert (0);
    }

    // Parse function call
    if (lexer_peek(0).type == TOKEN_TYPE_OPEN_BRACKET)
        return parse_function_call(expression);

    return expression;
}

static Expression *parse_expression(SymbolTable *table)
{
    Expression *left = parse_term(table);
    while (lexer_peek(0).type == TOKEN_TYPE_ADD)
    {
        match(TOKEN_TYPE_ADD);
        Expression *operation = malloc(sizeof(Expression));
        operation->type = EXPRESSION_TYPE_ADD;
        operation->left = left;
        operation->right = parse_term(table);
        left = operation;
    }

    return left;
}

static void parse_declaration(Scope *scope)
{
    // Create statement
    Statement statement;
    statement.type = STATEMENT_TYPE_DECLARATION;
    statement.data_type = parse_data_type();
    statement.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    statement.expression = NULL;

    // Create symbol
    Symbol symbol;
    symbol.name = statement.name;
    symbol.data_type = statement.data_type;
    symbol.flags = SYMBOL_LOCAL;
    symbol.params = NULL;
    symbol.param_count = 0;
    symbol_table_add(&scope->table, symbol);

    // Parse assignment expression if there is one
    if (lexer_peek(0).type == TOKEN_TYPE_EQUALS)
    {
        match(TOKEN_TYPE_EQUALS);
        statement.expression = parse_expression();
    }
    match(TOKEN_TYPE_SEMI);

    add_statement_to_scope(scope, statement);
}

static int is_data_type_next()
{
    Token token = lexer_peek(0);
    switch (token.type)
    {
        case TOKEN_TYPE_CONST:
            return 1;
        case TOKEN_TYPE_IDENTIFIER:
            if (lexer_compair_token_name(&token, "int"))
                return 1;
            if (lexer_compair_token_name(&token, "float"))
                return 1;
            return 0;
        default:
            return 0;
    }
}

static void parse_expression_statement(Scope *scope)
{
    Statement statement;
    statement.type = STATEMENT_TYPE_EXPRESSION;
    statement.expression = parse_expression();
    match(TOKEN_TYPE_SEMI);

    add_statement_to_scope(scope, statement);
}

static Scope *parse_scope()
{
    Scope *scope = malloc(sizeof(Scope));
    scope->statements = NULL;
    scope->statement_count = 0;

    match(TOKEN_TYPE_OPEN_SQUIGGLY);
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_SQUIGGLY)
    {
        if (is_data_type_next())
            parse_declaration(scope);
        else
            parse_expression_statement(scope);
    }
    match(TOKEN_TYPE_CLOSE_SQUIGGLY);

    return scope;
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
    function.body = NULL;
    if (lexer_peek(0).type == TOKEN_TYPE_OPEN_SQUIGGLY)
        function.body = parse_scope();
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

void free_expression(Expression *expression)
{
    switch (expression->type)
    {
        case EXPRESSION_TYPE_ADD:
            free_expression(expression->left);
            free_expression(expression->right);
            free(expression->left);
            free(expression->right);
            break;
        case EXPRESSION_TYPE_FUNCTION_CALL:
            free_expression(expression->left);
            for (int i = 0; i < expression->argument_length; i++)
            {
                free_expression(expression->arguments[i]);
                free(expression->arguments[i]);
            }
            free(expression->left);
            free(expression->arguments);
            break;
        default:
            break;
    }
}

void free_scope(Scope *scope)
{
    free_symbol_table(&scope->table);
    if (scope->statements)
    {
        for (int i = 0; i < scope->statement_count; i++)
        {
            Statement *statement = &scope->statements[i];
            if (statement->expression)
            {
                free_expression(statement->expression);
                free(statement->expression);
            }
        }
        free(scope->statements);
    }
}

void free_function(Function *function)
{
    if (function->body)
    {
        free_symbol_table(&function->body->table);
        free(function->body);
    }
}

void free_unit(Unit *unit)
{
    if (unit->functions)
    {
        for (int i = 0; i < unit->function_count; i++)
            free_function(&unit->functions[i]);
        free(unit->functions);
    }

    free_symbol_table(&unit->global_table);
}
