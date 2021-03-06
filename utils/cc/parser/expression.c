#include "../preprocessor/buffer.h"
#include "../dumpast.h"
#include "expression.h"
#include "parser.h"
#include "unit.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

static Expression *parse_unary_operator(SymbolTable *table, DataType *lhs_data_type);

const char *expression_type_name(enum ExpressionType type)
{
    switch (type)
    {
#define __TYPE(x) case EXPRESSION_TYPE_##x: return #x;
        ENUMERATE_EXPRESSION_TYPE
#undef __TYPE
    }
    return "UNKOWN";
}

enum Primitive primitive_from_name(Token *name)
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

int is_data_type_next(SymbolTable *table)
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

static Expression *parse_function_call(SymbolTable *table, Expression *left)
{
    Symbol *function_symbol = left->value.v;
    int argument_count = 0;

    // Create call expression
    Expression *call = malloc(sizeof(Expression));
    call->type = EXPRESSION_TYPE_FUNCTION_CALL;
    call->left = left;

    // Parse arguments
    BUFFER_DECLARE(argument_buffer, Expression*);
    BUFFER_INIT(argument_buffer, Expression*);

    match(TOKEN_TYPE_OPEN_BRACKET, "(");
    while (lexer_peek(0).type != TOKEN_TYPE_CLOSE_BRACKET)
    {
        Expression *argument = parse_expression(table);
        BUFFER_ADD(argument_buffer, Expression*, &argument);
        argument_count += 1;

        if (lexer_peek(0).type != TOKEN_TYPE_COMMA)
            break;
        match(TOKEN_TYPE_COMMA, ",");
    }
    match(TOKEN_TYPE_CLOSE_BRACKET, ")");
    call->arguments = argument_buffers;
    call->argument_length = argument_buffer_count;

    if (argument_count != function_symbol->param_count)
    {
        if (function_symbol->is_variadic)
        {
            // If this is a variadic function, the argument 
            // count must be more then the parameter count
            if (argument_count < function_symbol->param_count)
            {
                ERROR(NULL, "Expected at least %i argument(s) for variadic function '%s', got %i instead",
                    function_symbol->param_count, 
                    lexer_printable_token_data(&function_symbol->name), 
                    argument_count);
            }
        }
        else
        {
            ERROR(NULL, "Expected %i argument(s) to function '%s', got %i instead",
                function_symbol->param_count, 
                lexer_printable_token_data(&function_symbol->name), 
                argument_count);
        }
    }

    call->data_type = function_symbol->data_type;
    return call;
}

static Expression *parse_term(SymbolTable *table, DataType *lhs_data_type)
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
            expression->data_type = dt_int();
            break;
        
        case TOKEN_TYPE_FLOAT:
            value->type = VALUE_TYPE_FLOAT;
            value->f = atof(token.data);
            lexer_consume(TOKEN_TYPE_FLOAT);
            expression->data_type = dt_float();
            break;
        
        case TOKEN_TYPE_STRING:
            value->type = VALUE_TYPE_STRING;
            value->s = token;
            lexer_consume(TOKEN_TYPE_STRING);
            expression->data_type = dt_const_char_pointer();
            break;
        
        case TOKEN_TYPE_CHAR:
            value->type = VALUE_TYPE_CHAR;
            value->s = token;
            lexer_consume(TOKEN_TYPE_CHAR);
            expression->data_type = dt_char();
            break;

        case TOKEN_TYPE_IDENTIFIER:
            value->type = VALUE_TYPE_VARIABLE;
            value->v = symbol_table_lookup(table, &token);
            lexer_consume(TOKEN_TYPE_IDENTIFIER);
            if (!value->v)
            {
                if (lhs_data_type && lhs_data_type->flags & DATA_TYPE_STRUCT)
                    value->v = symbol_table_lookup(lhs_data_type->members, &token);

                if (!value->v)
                {
                    ERROR(&token, "No symbol with the name '%s' found",
                        lexer_printable_token_data(&token));
                    break;
                }
            }
            expression->data_type = value->v->data_type;
            break;
        
        default:
            ERROR(&token, "Expected expression, got '%s' instead",
                lexer_printable_token_data(&token));
            assert (0);
            lexer_consume(token.type);
            break;
    }

    // Parse function call
    if (lexer_peek(0).type == TOKEN_TYPE_OPEN_BRACKET)
        return parse_function_call(table, expression);

    return expression;
}

static Expression *make_unary_expression(
        SymbolTable *table, enum ExpressionType type, DataType *lhs_data_type)
{
    Expression *expression = malloc(sizeof(Expression));
    expression->left = parse_unary_operator(table, lhs_data_type);
    expression->type = type;

    expression->data_type = expression->left->data_type;
    if (type == EXPRESSION_TYPE_REF)
        expression->data_type.pointer_count += 1;
    return expression;
}

static Expression *parse_sub_expression_or_cast(
        SymbolTable *table, DataType *lhs_data_type)
{
    match(TOKEN_TYPE_OPEN_BRACKET, "(");
    if (is_data_type_next(table))
    {
        Expression *expression = malloc(sizeof(Expression));
        expression->data_type = parse_data_type(table);
        match(TOKEN_TYPE_CLOSE_BRACKET, ")");
        
        expression->left = parse_unary_operator(table, lhs_data_type);
        expression->type = EXPRESSION_TYPE_CAST;
        return expression;
    }

    Expression *sub_expression = parse_expression(table);
    match(TOKEN_TYPE_CLOSE_BRACKET, ")");
    return sub_expression;
}

static Expression *parse_unary_operator(
        SymbolTable *table, DataType *lhs_data_type)
{
    switch (lexer_peek(0).type)
    {
        case TOKEN_TYPE_AND:
            match(TOKEN_TYPE_AND, "&");
            return make_unary_expression(table, EXPRESSION_TYPE_REF, lhs_data_type);

        case TOKEN_TYPE_SUBTRACT:
            match(TOKEN_TYPE_SUBTRACT, "-");
            return make_unary_expression(table, EXPRESSION_TYPE_INVERT, lhs_data_type);
        
        case TOKEN_TYPE_OPEN_BRACKET:
            return parse_sub_expression_or_cast(table, lhs_data_type);
        
        default:
            return parse_term(table, lhs_data_type);
    }
}

static DataType find_data_type_of_operation(
    DataType left, enum ExpressionType operation, DataType right)
{
    (void) operation;

    if (!(left.flags & DATA_TYPE_PRIMITIVE) || !(right.flags & DATA_TYPE_PRIMITIVE))
        assert (0); // FIXME: Handle this

    // If one of them is signed, make them both signed
    left.flags &= ~(right.flags & DATA_TYPE_UNSIGNED);
    right.flags &= ~(left.flags & DATA_TYPE_UNSIGNED);

    switch (left.primitive)
    {
        case PRIMITIVE_INT:
            switch (right.primitive)
            {
                case PRIMITIVE_INT:
                    return left;
                case PRIMITIVE_CHAR:
                    return left;
                case PRIMITIVE_FLOAT:
                    return right;
                case PRIMITIVE_DOUBLE:
                    return right;
                default:
                    break;
            }
            break;
        case PRIMITIVE_CHAR:
            switch (right.primitive)
            {
                case PRIMITIVE_INT:
                    return right;
                case PRIMITIVE_CHAR:
                    return left;
                case PRIMITIVE_FLOAT:
                    return right;
                case PRIMITIVE_DOUBLE:
                    return right;
                default:
                    break;
            }
            break;
        case PRIMITIVE_FLOAT:
            switch (right.primitive)
            {
                case PRIMITIVE_INT:
                    return left;
                case PRIMITIVE_CHAR:
                    return left;
                case PRIMITIVE_FLOAT:
                    return left;
                case PRIMITIVE_DOUBLE:
                    return right;
                default:
                    break;
            }
            break;
        case PRIMITIVE_DOUBLE:
            switch (right.primitive)
            {
                case PRIMITIVE_INT:
                    return left;
                case PRIMITIVE_CHAR:
                    return left;
                case PRIMITIVE_FLOAT:
                    return left;
                case PRIMITIVE_DOUBLE:
                    return left;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    printf("Invalid operation\n");
    assert (0);
}

static Expression *create_operation_expression(Expression *left, enum ExpressionType op, Expression *right)
{
    Expression *operation = malloc(sizeof(Expression));
    operation->type = op;
    operation->left = left;
    operation->right = right;
    operation->common_type = find_data_type_of_operation(
        left->data_type, op, right->data_type);
    operation->data_type = operation->common_type;

    // If it's a boolean operation, it always returns an int
    if (op == EXPRESSION_TYPE_LESS_THAN ||
        op == EXPRESSION_TYPE_GREATER_THAN ||
        op == EXPRESSION_TYPE_NOT_EQUALS)
    {
        operation->data_type = dt_int();
    }
    return operation;
}

static enum ExpressionType expression_type_from_token_type(enum TokenType token_type)
{
    switch (token_type)
    {
        case TOKEN_TYPE_ADD:
            return EXPRESSION_TYPE_ADD;
        case TOKEN_TYPE_SUBTRACT:
            return EXPRESSION_TYPE_SUB;
        case TOKEN_TYPE_STAR:
            return EXPRESSION_TYPE_MUL;
        case TOKEN_TYPE_FORWARD_SLASH:
            return EXPRESSION_TYPE_DIV;
        case TOKEN_TYPE_DOT:
            return EXPRESSION_TYPE_DOT;
        case TOKEN_TYPE_OPEN_SQUARE:
            return EXPRESSION_TYPE_INDEX;
        case TOKEN_TYPE_LESS_THAN:
            return EXPRESSION_TYPE_LESS_THAN;
        case TOKEN_TYPE_GREATER_THAN:
            return EXPRESSION_TYPE_GREATER_THAN;
        case TOKEN_TYPE_NOT_EQUALS:
            return EXPRESSION_TYPE_NOT_EQUALS;
        default:
            assert (0);
    }
}

static Expression *parse_access_op(SymbolTable *table, DataType *lhs_data_type)
{
    Expression *left = parse_unary_operator(table, lhs_data_type);

    enum TokenType operation_type = lexer_peek(0).type;
    while (operation_type == TOKEN_TYPE_DOT || operation_type == TOKEN_TYPE_OPEN_SQUARE)
    {
        lexer_consume(operation_type);
        enum ExpressionType op = expression_type_from_token_type(operation_type);
        Expression *right = parse_unary_operator(table, &left->data_type);
        left = create_operation_expression(left, op, right);
        left->data_type = right->data_type;

        // If this is an index operation, check for a closing square
        if (operation_type == TOKEN_TYPE_OPEN_SQUARE && operation_type == TOKEN_TYPE_OPEN_SQUARE)
            match(TOKEN_TYPE_CLOSE_SQUARE, "]");
        operation_type = lexer_peek(0).type;
    }

    return left;
}

static Expression *parse_mul_op(SymbolTable *table, DataType *lhs_data_type)
{
    Expression *left = parse_access_op(table, lhs_data_type);

    enum TokenType operation_type = lexer_peek(0).type;
    while (operation_type == TOKEN_TYPE_STAR || operation_type == TOKEN_TYPE_FORWARD_SLASH)
    {
        lexer_consume(operation_type);
        enum ExpressionType op = expression_type_from_token_type(operation_type);
        Expression *right = parse_access_op(table, &left->data_type);
        left = create_operation_expression(left, op, right);

        operation_type = lexer_peek(0).type;
    }

    return left;
}

static Expression *parse_add_op(SymbolTable *table, DataType *lhs_data_type)
{
    Expression *left = parse_mul_op(table, lhs_data_type);

    enum TokenType operation_type = lexer_peek(0).type;
    while (operation_type == TOKEN_TYPE_ADD || operation_type == TOKEN_TYPE_SUBTRACT)
    {
        lexer_consume(operation_type);
        enum ExpressionType op = expression_type_from_token_type(operation_type);
        Expression *right = parse_mul_op(table, &left->data_type);
        left = create_operation_expression(left, op, right);

        operation_type = lexer_peek(0).type;
    }

    return left;
}

static Expression *parse_logical(SymbolTable *table, DataType *lhs_data_type)
{
    Expression *left = parse_add_op(table, lhs_data_type);

    enum TokenType operation_type = lexer_peek(0).type;
    while (operation_type == TOKEN_TYPE_LESS_THAN ||
           operation_type == TOKEN_TYPE_GREATER_THAN ||
           operation_type == TOKEN_TYPE_NOT_EQUALS)
    {
        lexer_consume(operation_type);
        enum ExpressionType op = expression_type_from_token_type(operation_type);
        Expression *right = parse_add_op(table, &left->data_type);
        left = create_operation_expression(left, op, right);

        operation_type = lexer_peek(0).type;
    }

    return left;
}

void check_assign_types(
    DataType *left, DataType *right)
{
    if (!data_type_equals(left, right))
    {
        char msg[80];
        sprintf(msg, "Cannot assign type '%s' to ", printable_data_type(right));
        sprintf(msg + strlen(msg), "type '%s'\n", printable_data_type(left));
        ERROR(NULL, msg);
    }
}

Expression *parse_expression(SymbolTable *table)
{
    Expression *left = parse_logical(table, NULL);

    while (lexer_peek(0).type == TOKEN_TYPE_EQUALS)
    {
        match(TOKEN_TYPE_EQUALS, "=");
        Expression *right = parse_logical(table, &left->data_type);

        check_assign_types(&left->data_type, &right->data_type);
        left = create_operation_expression(left, EXPRESSION_TYPE_ASSIGN, right);
    }

    return left;
}

