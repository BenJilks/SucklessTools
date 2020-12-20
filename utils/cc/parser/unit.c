#include "../preprocessor/buffer.h"
#include "unit.h"
#include <stdlib.h>

// Global state

BUFFER_DECLARE(function, Function);
BUFFER_DECLARE(struct, Struct);
BUFFER_DECLARE(enum, Enum);

SymbolTable *global_table;

// Functions
static void free_expression(Expression *expression);
static void free_scope(Scope *scope);
static void free_function(Function *function);
static void free_struct(Struct *struct_);

void unit_create()
{
    BUFFER_INIT(function, Function);
    BUFFER_INIT(struct, Struct);
    BUFFER_INIT(enum, Enum);
    global_table = symbol_table_new(NULL);
}

void unit_destroy()
{
    for (int i = 0; i < function_count; i++)
        free_function(&functions[i]);
    free(functions);

    for (int i = 0; i < struct_count; i++)
        free_struct(&structs[i]);
    free(structs);

    free(enums);
    free_symbol_table(global_table);
}

Scope *scope_create(SymbolTable *parent)
{
    Scope *scope = malloc(sizeof(Scope));
    scope->statements = NULL;
    scope->statement_count = 0;
    scope->table = symbol_table_new(parent);
    return scope;
}

void add_statement_to_scope(Scope *scope, Statement statement)
{
    if (!scope->statements)
        scope->statements = malloc(sizeof(Statement));
    else
        scope->statements = realloc(scope->statements, (scope->statement_count + 1) * sizeof(Statement));

    scope->statements[scope->statement_count] = statement;
    scope->statement_count += 1;
}

Statement statement_create()
{
    Statement statement;
    statement.type = STATEMENT_TYPE_NULL;
    statement.expression = NULL;
    statement.next = NULL;
    statement.sub_scope = NULL;
    return statement;
}

void unit_add_function(Function *func)
{
    BUFFER_ADD(function, Function, func);
}

void unit_add_struct(Struct *struct_)
{
    BUFFER_ADD(struct, Struct, struct_);
}

void unit_add_enum(Enum *enum_)
{
    BUFFER_ADD(enum, Enum, enum_);
}

Struct *unit_find_struct(Token *name)
{
    for (int i = 0; i < struct_count; i++)
    {
        if (lexer_compair_token_token(&structs[i].name, name))
            return &structs[i];
    }

    return NULL;
}

static void free_expression(Expression *expression)
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

static void free_scope(Scope *scope)
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

static void free_function(Function *function)
{
    free_symbol_table(function->table);
    free(function->table);
    if (function->body)
    {
        free_scope(function->body);
        free(function->body);
    }
}

static void free_struct(Struct *struct_)
{
    free_symbol_table(struct_->members);
    free(struct_->members);
}

