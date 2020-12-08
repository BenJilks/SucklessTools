#include "parser.h"
#include "lexer.h"
#include "unit.h"
#include "buffer.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

static void parse_statement(Function*, Scope*);
static Scope *parse_scope(Function*, SymbolTable *parent);

Token match(enum TokenType type, const char *name)
{
    Token token = lexer_consume(type);
    if (type != TOKEN_TYPE_NONE && token.type == TOKEN_TYPE_NONE)
    {
        Token got = lexer_peek(0);
        ERROR(&got, "Expected token '%s', got '%s' instead",
            name, lexer_printable_token_data(&got));
    }
    return token;
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

static void parse_declaration(Function *function, Scope *scope)
{
    DataType data_type = parse_data_type(scope->table);
    Statement statement;

    Statement *curr_statement = NULL;
    for (;;)
    {
        if (curr_statement == NULL)
        {
            curr_statement = &statement;
            curr_statement->next = NULL;
        }
        else
        {
            Statement *next_statement = malloc(sizeof(Statement));
            curr_statement->next = next_statement;
            curr_statement = next_statement;
            curr_statement->next = NULL;
        }

        // Create symbol
        Symbol symbol;
        symbol.data_type = data_type;
        symbol.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
        symbol.flags = SYMBOL_LOCAL;
        symbol.location = function->stack_size;
        symbol.params = NULL;
        symbol.param_count = 0;

        // Parse array size, if there is any
        DataType *symbol_type = &symbol.data_type;
        while (lexer_peek(0).type == TOKEN_TYPE_OPEN_SQUARE)
        {
            match(TOKEN_TYPE_OPEN_SQUARE, "["); 
            Token count_token = lexer_consume(TOKEN_TYPE_INTEGER);

            int array_size = atoi(lexer_printable_token_data(&count_token));
            symbol_type->array_sizes[symbol_type->array_count++] = array_size;
            
            match(TOKEN_TYPE_CLOSE_SQUARE, "]");
        }

        symbol_table_add(scope->table, symbol);
        function->stack_size += symbol_size(&symbol);

        // Create statement
        curr_statement->type = STATEMENT_TYPE_DECLARATION;
        curr_statement->symbol = symbol;
        curr_statement->expression = NULL;

        // Parse assignment expression if there is one
        if (lexer_peek(0).type == TOKEN_TYPE_EQUALS)
        {
            match(TOKEN_TYPE_EQUALS, "=");
            curr_statement->expression = parse_expression(scope->table);
        }

        if (lexer_peek(0).type != TOKEN_TYPE_COMMA)
            break;
        match(TOKEN_TYPE_COMMA, ",");
    }
    match(TOKEN_TYPE_SEMI, ";");

    add_statement_to_scope(scope, statement);
}

static void parse_expression_statement(Scope *scope)
{
    Statement statement;
    statement.type = STATEMENT_TYPE_EXPRESSION;
    statement.expression = parse_expression(scope->table);
    statement.sub_scope = NULL;
    match(TOKEN_TYPE_SEMI, ";");

    add_statement_to_scope(scope, statement);
}

static void parse_return(Scope *scope)
{
    match(TOKEN_TYPE_RETURN, "return");

    Statement statement;
    statement.type = STATEMENT_TYPE_RETURN;
    statement.expression = parse_expression(scope->table);
    statement.sub_scope = NULL;
    match(TOKEN_TYPE_SEMI, ";");

    add_statement_to_scope(scope, statement);
}

static Scope *parse_block_or_statement(Function *function, SymbolTable *parent)
{
    if (lexer_peek(0).type == TOKEN_TYPE_OPEN_SQUIGGLY)
        return parse_scope(function, parent);

    Scope *scope = malloc(sizeof(Scope));
    scope->statements = NULL;
    scope->statement_count = 0;
    scope->table = symbol_table_new(parent);
    parse_statement(function, scope);
    return scope;
}

static void parse_if(Function *function, Scope *scope)
{
    match(TOKEN_TYPE_IF, "if");
    match(TOKEN_TYPE_OPEN_BRACKET, "(");

    Statement statement;
    statement.type = STATEMENT_TYPE_IF;
    statement.expression = parse_expression(scope->table);
    match(TOKEN_TYPE_CLOSE_BRACKET, ")");

    statement.sub_scope = parse_block_or_statement(function, scope->table);
    add_statement_to_scope(scope, statement);
}

static void parse_while(Function *function, Scope *scope)
{
    match(TOKEN_TYPE_WHILE, "while");
    match(TOKEN_TYPE_OPEN_BRACKET, "(");

    Statement statement;
    statement.type = STATEMENT_TYPE_WHILE;
    statement.expression = parse_expression(scope->table);
    match(TOKEN_TYPE_CLOSE_BRACKET, ")");

    statement.sub_scope = parse_scope(function, scope->table);
    add_statement_to_scope(scope, statement);
}

static void parse_typedef(SymbolTable *table)
{
    match(TOKEN_TYPE_TYPEDEF, "typedef");
    DataType data_type = parse_data_type(table);
    Token name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    match(TOKEN_TYPE_SEMI, ";");
    symbol_table_define_type(table, name, data_type);
}

static void parse_statement(Function *function, Scope *scope)
{
    if (is_data_type_next(scope->table))
    {
        parse_declaration(function, scope);
        return;
    }

    switch (lexer_peek(0).type)
    {
        case TOKEN_TYPE_RETURN:
            parse_return(scope);
            break;
        case TOKEN_TYPE_IF:
            parse_if(function, scope);
            break;
        case TOKEN_TYPE_WHILE:
            parse_while(function, scope);
            break;
        case TOKEN_TYPE_TYPEDEF:
            parse_typedef(scope->table);
            break;
        default:
            parse_expression_statement(scope);
            break;
    }
}

static Scope *parse_scope(Function *function, SymbolTable *parent)
{
    Scope *scope = malloc(sizeof(Scope));
    scope->statements = NULL;
    scope->statement_count = 0;
    scope->table = symbol_table_new(parent);

    match(TOKEN_TYPE_OPEN_SQUIGGLY, "{");
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_SQUIGGLY)
        parse_statement(function, scope);
    match(TOKEN_TYPE_CLOSE_SQUIGGLY, "}");

    return scope;
}

static void parse_params(Function *function, Symbol *symbol)
{
    BUFFER_DECLARE(param, Symbol);
    BUFFER_INIT(param, Symbol);
    symbol->is_variadic = 0;

    match(TOKEN_TYPE_OPEN_BRACKET, "(");
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_BRACKET)
    {
        if (lexer_peek(0).type == TOKEN_TYPE_ELLIPSE)
        {
            match(TOKEN_TYPE_ELLIPSE, "...");
            symbol->is_variadic = 1;
            break;
        }

        Symbol param;
        param.flags = SYMBOL_ARGUMENT;
        param.data_type = parse_data_type(function->table);
        param.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
        param.params = NULL;
        param.param_count = 0;

        param.location = function->argument_size;
        function->argument_size += symbol_size(&param);

        BUFFER_ADD(param, Symbol, &param);
        symbol_table_add(function->table, param);

        if (lexer_peek(0).type != TOKEN_TYPE_COMMA)
            break;
        match(TOKEN_TYPE_COMMA, ",");
    }
    match(TOKEN_TYPE_CLOSE_BRACKET, ")");
    symbol->params = params;
    symbol->param_count = param_count;
}

static void parse_function()
{
    if (!is_data_type_next(global_table))
    {
        Token token = lexer_consume(TOKEN_TYPE_NONE);
        ERROR(&token, "Expected datatype, got '%s' instead",
            lexer_printable_token_data(&token));
        return;
    }

    Function function;
    function.table = symbol_table_new(global_table);
    function.stack_size = 0;
    function.argument_size = 0;

    // Function definition
    Symbol func_symbol;
    func_symbol.data_type = parse_data_type(function.table);
    func_symbol.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    parse_params(&function, &func_symbol);

    // Register symbol
    symbol_table_add(global_table, func_symbol);

    // Optional function body
    function.func_symbol = func_symbol;
    function.body = NULL;
    if (lexer_peek(0).type == TOKEN_TYPE_OPEN_SQUIGGLY)
        function.body = parse_scope(&function, function.table);
    else
        match(TOKEN_TYPE_SEMI, ";");

    unit_add_function(&function);
}

static void parse_struct()
{
    match(TOKEN_TYPE_STRUCT, "struct");

    Struct struct_;
    struct_.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    struct_.members = symbol_table_new(NULL);

    int allocator = 0;
    match(TOKEN_TYPE_OPEN_SQUIGGLY, "{");
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_SQUIGGLY)
    {
        Symbol member;
        member.data_type = parse_data_type(global_table);
        member.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
        member.flags = SYMBOL_MEMBER;
        member.params = NULL;
        member.param_count = 0;
        member.is_variadic = 0;
        member.location = allocator;
        match(TOKEN_TYPE_SEMI, ";");

        allocator += data_type_size(&member.data_type);
        symbol_table_add(struct_.members, member);
    }
    match(TOKEN_TYPE_CLOSE_SQUIGGLY, "}");
    match(TOKEN_TYPE_SEMI, ";");

    unit_add_struct(&struct_);
}

static void parse_enum()
{
    match(TOKEN_TYPE_ENUM, "enum");

    Enum enum_;
    enum_.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    enum_.members = symbol_table_new(NULL);

    DataType enum_type;
    enum_type.name = enum_.name;
    enum_type.flags = DATA_TYPE_ENUM | DATA_TYPE_PRIMITIVE;
    enum_type.pointer_count = 0;
    enum_type.primitive = PRIMITIVE_INT;

    match(TOKEN_TYPE_OPEN_SQUIGGLY, "{");
    int auto_allocator = 0;
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_SQUIGGLY)
    {
        Symbol member;
        member.data_type = enum_type;
        member.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
        member.flags = SYMBOL_ENUM;
        member.param_count = 0;
        member.is_variadic = 0;
        member.params = NULL;

        // If there's an '==', we have a value
        if (lexer_peek(0).type == TOKEN_TYPE_EQUALS)
        {
            // TODO: We should allow constant expressions
            match(TOKEN_TYPE_EQUALS, "==");
            Token number = match(TOKEN_TYPE_INTEGER, "Enum Value");
            auto_allocator = atoi(lexer_printable_token_data(&number));
        }

        member.location = auto_allocator++;
        symbol_table_add(enum_.members, member);
        symbol_table_add(global_table, member);

        if (lexer_peek(0).type != TOKEN_TYPE_COMMA)
            break;
        match(TOKEN_TYPE_COMMA, ",");
    }
    match(TOKEN_TYPE_CLOSE_SQUIGGLY, "}");
    match(TOKEN_TYPE_SEMI, ";");
}

void parse()
{
    for (;;)
    {
        Token token = lexer_peek(0);
        if (token.type == TOKEN_TYPE_NONE)
            break;

        switch (token.type)
        {
            case TOKEN_TYPE_IDENTIFIER:
                parse_function();
                break;
            case TOKEN_TYPE_TYPEDEF:
                parse_typedef(global_table);
                break;
            case TOKEN_TYPE_STRUCT:
                parse_struct();
                break;
            case TOKEN_TYPE_ENUM:
                parse_enum();
                break;
            default:
                assert (0);
        }
    }
}

