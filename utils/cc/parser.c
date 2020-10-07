#include "parser.h"
#include "lexer.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

static Scope *parse_scope(Function *function, Unit *unit, SymbolTable *parent);

void match(enum TokenType type, const char *name)
{
    Token token = lexer_consume(type);
    if (type != TOKEN_TYPE_NONE && token.type == TOKEN_TYPE_NONE)
    {
        Token got = lexer_peek(0);
        ERROR("Expected token '%s', got '%s' instead",
            name, lexer_printable_token_data(&got));
    }
}

static enum Primitive primitive_from_name(Token *name)
{
    if (lexer_compair_token_name(name, "void"))
        return PRIMITIVE_VOID;
    else if (lexer_compair_token_name(name, "int"))
        return PRIMITIVE_INT;
    else if (lexer_compair_token_name(name, "float"))
        return PRIMITIVE_FLOAT;
    else if (lexer_compair_token_name(name, "double"))
        return PRIMITIVE_DOUBLE;
    else if (lexer_compair_token_name(name, "char"))
        return PRIMITIVE_CHAR;

    return PRIMITIVE_NONE;
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

#define PRIMITIVE_DATA_TYPE(name, primitive) \
    DataType dt_##name() \
    { \
        return (DataType){ { #name, sizeof(#name), TOKEN_TYPE_IDENTIFIER }, sizeof(name), DATA_TYPE_PRIMITIVE, primitive, NULL, 0 }; \
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
        ERROR("Expected datatype, got '%s' instead", lexer_printable_token_data(&data_type.name));

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
        ERROR("No struct with the name '%s' found\n",
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

static DataType parse_data_type(Unit *unit, SymbolTable *table)
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

Buffer make_buffer(void **memory, int *count, int unit_size)
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

void append_buffer(Buffer *buffer, void *item)
{
    if (*buffer->count >= buffer->buffer)
    {
        buffer->buffer += 80;
        buffer->memory = realloc(buffer->memory, buffer->buffer * buffer->unit_size);
    }

    memcpy(buffer->memory + (*buffer->count * buffer->unit_size), item, buffer->unit_size);
    *buffer->count += 1;
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

static void parse_declaration(Function *function, Unit *unit, Scope *scope)
{
    DataType data_type = parse_data_type(unit, scope->table);
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
        symbol.array_count = 1;

        // Parse array size, if there is any
        if (lexer_peek(0).type == TOKEN_TYPE_OPEN_SQUARE)
        {
            match(TOKEN_TYPE_OPEN_SQUARE, "[");
            Token count_token = lexer_consume(TOKEN_TYPE_INTEGER);
            symbol.array_count = atoi(lexer_printable_token_data(&count_token));
            symbol.flags |= SYMBOL_ARRAY;
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

static int is_data_type_next(SymbolTable *table)
{
    Token token = lexer_peek(0);
    switch (token.type)
    {
        case TOKEN_TYPE_CONST:
            return 1;
        case TOKEN_TYPE_STRUCT:
            return 1;
        case TOKEN_TYPE_UNSIGNED:
            return 1;
        case TOKEN_TYPE_IDENTIFIER:
            if (primitive_from_name(&token) != PRIMITIVE_NONE ||
                symbol_table_lookup_type(table, &token) != NULL)
            {
                return 1;
            }
            return 0;
        default:
            return 0;
    }
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

static void parse_if(Function *function, Unit *unit, Scope *scope)
{
    match(TOKEN_TYPE_IF, "if");
    match(TOKEN_TYPE_OPEN_BRACKET, "(");

    Statement statement;
    statement.type = STATEMENT_TYPE_IF;
    statement.expression = parse_expression(scope->table);
    match(TOKEN_TYPE_CLOSE_BRACKET, ")");

    statement.sub_scope = parse_scope(function, unit, scope->table);
    add_statement_to_scope(scope, statement);
}

static void parse_while(Function *function, Unit *unit, Scope *scope)
{
    match(TOKEN_TYPE_WHILE, "while");
    match(TOKEN_TYPE_OPEN_BRACKET, "(");

    Statement statement;
    statement.type = STATEMENT_TYPE_WHILE;
    statement.expression = parse_expression(scope->table);
    match(TOKEN_TYPE_CLOSE_BRACKET, ")");

    statement.sub_scope = parse_scope(function, unit, scope->table);
    add_statement_to_scope(scope, statement);
}

static void parse_typedef(Unit *unit, SymbolTable *table)
{
    match(TOKEN_TYPE_TYPEDEF, "typedef");
    DataType data_type = parse_data_type(unit, table);
    Token name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    match(TOKEN_TYPE_SEMI, ";");
    symbol_table_define_type(table, name, data_type);
}

static Scope *parse_scope(Function *function, Unit *unit, SymbolTable *parent)
{
    Scope *scope = malloc(sizeof(Scope));
    scope->statements = NULL;
    scope->statement_count = 0;
    scope->table = symbol_table_new(parent);

    match(TOKEN_TYPE_OPEN_SQUIGGLY, "{");
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_SQUIGGLY)
    {
        if (is_data_type_next(scope->table))
        {
            parse_declaration(function, unit, scope);
            continue;
        }

        switch (lexer_peek(0).type)
        {
            case TOKEN_TYPE_RETURN:
                parse_return(scope);
                break;
            case TOKEN_TYPE_IF:
                parse_if(function, unit, scope);
                break;
            case TOKEN_TYPE_WHILE:
                parse_while(function, unit, scope);
                break;
            case TOKEN_TYPE_TYPEDEF:
                parse_typedef(unit, scope->table);
                break;
            default:
                parse_expression_statement(scope);
                break;
        }
    }
    match(TOKEN_TYPE_CLOSE_SQUIGGLY, "}");

    return scope;
}

static void parse_params(Function *function, Unit *unit, Symbol *symbol)
{
    Buffer params_buffer = make_buffer((void**)&symbol->params, &symbol->param_count, sizeof(Symbol));
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
        param.data_type = parse_data_type(unit, function->table);
        param.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
        param.params = NULL;
        param.param_count = 0;

        param.location = function->argument_size;
        function->argument_size += symbol_size(&param);

        append_buffer(&params_buffer, &param);
        symbol_table_add(function->table, param);

        if (lexer_peek(0).type != TOKEN_TYPE_COMMA)
            break;
        match(TOKEN_TYPE_COMMA, ",");
    }
    match(TOKEN_TYPE_CLOSE_BRACKET, ")");
}

static void parse_function(Unit *unit)
{
    Function function;
    function.table = symbol_table_new(unit->global_table);
    function.stack_size = 0;
    function.argument_size = 0;

    // Function definition
    Symbol func_symbol;
    func_symbol.data_type = parse_data_type(unit, function.table);
    func_symbol.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
    parse_params(&function, unit, &func_symbol);

    // Register symbol
    symbol_table_add(unit->global_table, func_symbol);

    // Optional function body
    function.func_symbol = func_symbol;
    function.body = NULL;
    if (lexer_peek(0).type == TOKEN_TYPE_OPEN_SQUIGGLY)
        function.body = parse_scope(&function, unit, function.table);
    else
        match(TOKEN_TYPE_SEMI, ";");

    if (!unit->functions)
        unit->functions = malloc(sizeof(Function));
    else
        unit->functions = realloc(unit->functions, (unit->function_count + 1) * sizeof(Function));
    unit->functions[unit->function_count++] = function;
}

static void parse_struct(Unit *unit)
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
        member.data_type = parse_data_type(unit, unit->global_table);
        member.name = lexer_consume(TOKEN_TYPE_IDENTIFIER);
        member.flags = SYMBOL_MEMBER;
        member.params = NULL;
        member.param_count = 0;
        member.is_variadic = 0;
        member.location = allocator;
        match(TOKEN_TYPE_SEMI, ";");

        allocator += member.data_type.size;
        symbol_table_add(struct_.members, member);
    }
    match(TOKEN_TYPE_CLOSE_SQUIGGLY, "}");
    match(TOKEN_TYPE_SEMI, ";");

    if (!unit->structs)
        unit->structs = malloc(sizeof(Struct));
    else
        unit->structs = realloc(unit->structs, (unit->struct_count + 1) * sizeof(Struct));
    unit->structs[unit->struct_count++] = struct_;
}

Unit *parse()
{
    Unit *unit = malloc(sizeof (Unit));
    unit->functions = NULL;
    unit->function_count = 0;
    unit->structs = NULL;
    unit->struct_count = 0;
    unit->global_table = symbol_table_new(NULL);

    for (;;)
    {
        Token token = lexer_peek(0);
        if (token.type == TOKEN_TYPE_NONE)
            break;

        switch (token.type)
        {
            case TOKEN_TYPE_IDENTIFIER:
                parse_function(unit);
                break;
            case TOKEN_TYPE_TYPEDEF:
                parse_typedef(unit, unit->global_table);
                break;
            case TOKEN_TYPE_STRUCT:
                parse_struct(unit);
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
        case EXPRESSION_TYPE_REF:
            free_expression(expression->left);
            free(expression->left);
            break;
        default:
            break;
    }
}

void free_scope(Scope *scope)
{
    free_symbol_table(scope->table);
    free(scope->table);
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
            if (statement->sub_scope)
            {
                free_scope(statement->sub_scope);
                free(statement->sub_scope);
            }
        }
        free(scope->statements);
    }
}

void free_function(Function *function)
{
    free_symbol_table(function->table);
    free(function->table);
    if (function->body)
    {
        free_symbol_table(function->body->table);
        free(function->body);
    }
}

void free_struct(Struct *struct_)
{
    free_symbol_table(struct_->members);
    free(struct_->members);
}

void free_unit(Unit *unit)
{
    if (unit->functions)
    {
        for (int i = 0; i < unit->function_count; i++)
            free_function(&unit->functions[i]);
        free(unit->functions);
    }
    if (unit->structs)
    {
        for (int i = 0; i < unit->struct_count; i++)
            free_struct(&unit->structs[i]);
        free(unit->structs);
    }

    free_symbol_table(unit->global_table);
    free(unit->global_table);
}
