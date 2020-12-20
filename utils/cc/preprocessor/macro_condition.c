#include "macro_condition.h"
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

// #define DEBUG_PRE_PROCCESSOR

#ifdef DEBUG_PRE_PROCCESSOR
static int g_debug_indent = 0;
#define DEBUG(...) \
    do { \
        for (int i = 0; i < g_debug_indent; i++) fprintf(stderr, "  "); \
            fprintf(stderr, __VA_ARGS__); \
    } while (0)
#define DEBUG_START_SCOPE g_debug_indent++
#define DEBUG_END_SCOPE g_debug_indent--
#else
#define DEBUG(...)
#define DEBUG_START_SCOPE
#define DEBUG_END_SCOPE
#endif

enum TokenType
{
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_NOT,
    TOKEN_TYPE_OR,
    TOKEN_TYPE_AND,
    TOKEN_TYPE_EQUALS,
    TOKEN_TYPE_MORE_THAN,
    TOKEN_TYPE_LESS_THAN,
    TOKEN_TYPE_MORE_THAN_OR_EQUALS,
    TOKEN_TYPE_LESS_THAN_OR_EQUALS,
    TOKEN_TYPE_SUBTRACT,
    TOKEN_TYPE_ADD,
    TOKEN_TYPE_TERNARY,
    TOKEN_TYPE_TERNARY_ELSE,
    TOKEN_TYPE_OPEN_BRACKET,
    TOKEN_TYPE_CLOSE_BRACKET,
};

enum State
{
    STATE_DEFAULT,
    STATE_IDENTIFIER,
    STATE_NUMBER,
    STATE_PIPE,
    STATE_ESCAPE,
    STATE_AND,
    STATE_EQUALS,
    STATE_GREATER_THAN,
    STATE_LESS_THAN,
};

typedef struct Token
{
    char data[80];
    enum TokenType type;
} Token;

static int parse_condition(Stream *input, Token *token);

static Token next_token(Stream *input)
{
    enum State state = STATE_DEFAULT;
    int buffer_pointer = 0;
    Token token;
    for (;;)
    {
        stream_read_next_char(input);
        switch (state)
        {
            case STATE_DEFAULT:
                if (input->peek == (char)EOF)
                    return (Token) { {}, TOKEN_TYPE_EOF };
                if (isspace(input->peek))
                    break;
                if (isdigit(input->peek))
                {
                    state = STATE_NUMBER;
                    input->should_reconsume = 1;
                    break;
                }
                if (isalpha(input->peek) || input->peek == '_')
                {
                    state = STATE_IDENTIFIER;
                    input->should_reconsume = 1;
                    break;
                }
                switch (input->peek)
                {
                    case '!':
                        token.data[0] = '!';
                        token.data[1] = '\0';
                        token.type = TOKEN_TYPE_NOT;
                        return token;
                    case '|':
                        state = STATE_PIPE;
                        break;
                    case '&':
                        state = STATE_AND;
                        break;
                    case '\\':
                        state = STATE_ESCAPE;
                        break;
                    case '=':
                        state = STATE_EQUALS;
                        break;
                    case '>':
                        state = STATE_GREATER_THAN;
                        break;
                    case '<':
                        state = STATE_LESS_THAN;
                        break;
                    case '-':
                        strcpy(token.data, "-");
                        token.type = TOKEN_TYPE_SUBTRACT;
                        return token;
                    case '+':
                        strcpy(token.data, "+");
                        token.type = TOKEN_TYPE_ADD;
                        return token;
                    case '?':
                        strcpy(token.data, "?");
                        token.type = TOKEN_TYPE_TERNARY;
                        return token;
                    case ':':
                        strcpy(token.data, ":");
                        token.type = TOKEN_TYPE_TERNARY_ELSE;
                        return token;
                    case '(':
                        strcpy(token.data, "(");
                        token.type = TOKEN_TYPE_OPEN_BRACKET;
                        return token;
                    case ')':
                        strcpy(token.data, ")");
                        token.type = TOKEN_TYPE_CLOSE_BRACKET;
                        return token;
                    default:
                        assert (0);
                }
                break;
            case STATE_NUMBER:
                if (!isdigit(input->peek) && input->peek != 'L')
                {
                    token.data[buffer_pointer] = '\0';
                    token.type = TOKEN_TYPE_NUMBER;
                    state = STATE_DEFAULT;
                    input->should_reconsume = 1;
                    return token;
                }
                token.data[buffer_pointer++] = input->peek;
                break;
            case STATE_IDENTIFIER:
                if (!isalnum(input->peek) && input->peek != '_')
                {
                    token.data[buffer_pointer] = '\0';
                    token.type = TOKEN_TYPE_IDENTIFIER;
                    state = STATE_DEFAULT;
                    input->should_reconsume = 1;
                    return token;
                }
                token.data[buffer_pointer++] = input->peek;
                break;
            case STATE_PIPE:
                if (input->peek == '|')
                {
                    strcpy(token.data, "||");
                    token.type = TOKEN_TYPE_OR;
                    state = STATE_DEFAULT;
                    return token;
                }
                assert (0);
                break;
            case STATE_AND:
                if (input->peek == '&')
                {
                    strcpy(token.data, "&&");
                    token.type = TOKEN_TYPE_AND;
                    state = STATE_DEFAULT;
                    return token;
                }
                assert (0);
                break;
            case STATE_EQUALS:
                if (input->peek == '=')
                {
                    strcpy(token.data, "==");
                    token.type = TOKEN_TYPE_EQUALS;
                    state = STATE_DEFAULT;
                    return token;
                }
                assert (0);
                break;
            case STATE_GREATER_THAN:
                if (input->peek == '=')
                {
                    strcpy(token.data, ">=");
                    token.type = TOKEN_TYPE_MORE_THAN_OR_EQUALS;
                    state = STATE_DEFAULT;
                    return token;
                }
                strcpy(token.data, ">");
                token.type = TOKEN_TYPE_MORE_THAN;
                state = STATE_DEFAULT;
                input->should_reconsume = 1;
                return token;
            case STATE_LESS_THAN:
                if (input->peek == '=')
                {
                    strcpy(token.data, "<=");
                    token.type = TOKEN_TYPE_LESS_THAN_OR_EQUALS;
                    state = STATE_DEFAULT;
                    return token;
                }
                strcpy(token.data, "<");
                token.type = TOKEN_TYPE_LESS_THAN;
                state = STATE_DEFAULT;
                input->should_reconsume = 1;
                return token;
            case STATE_ESCAPE:
                if (input->peek == '\n')
                {
                    state = STATE_DEFAULT;
                    break;
                }
                assert (0);
                break;
        }
    }

    return (Token) { {}, TOKEN_TYPE_EOF };
}

static int parse_condition_term(Stream *input, Token *token)
{
    *token = next_token(input);
    switch (token->type)
    {
        case TOKEN_TYPE_NUMBER:
        {
            int number = atoi(token->data);
            DEBUG("Number: %i\n", number);
            *token = next_token(input);
            return number;
        }
        case TOKEN_TYPE_IDENTIFIER:
        {
            DEBUG("Undefined: %s\n", token->data);
            *token = next_token(input);
            return 0;
        }
        case TOKEN_TYPE_NOT:
            DEBUG("Not\n");
            return !parse_condition_term(input, token);
        case TOKEN_TYPE_OPEN_BRACKET:
        {
            DEBUG("Sub condition\n");
            DEBUG_START_SCOPE;
            int condition_result = parse_condition(input, token);
            assert (token->type == TOKEN_TYPE_CLOSE_BRACKET);
            DEBUG_END_SCOPE;
            DEBUG("End sub condition = %i\n", condition_result);
            *token = next_token(input);
            return condition_result;
        }
        default:
            printf("%s - %i\n", token->data, token->type);
            assert (0);
    }
}

static int parse_factor(Stream *input, Token *token)
{
    int condition_result = parse_condition_term(input, token);
    switch (token->type)
    {
        case TOKEN_TYPE_SUBTRACT:
            DEBUG("-\n");
            return condition_result - parse_factor(input, token);
        case TOKEN_TYPE_ADD:
            DEBUG("+\n");
            return condition_result + parse_factor(input, token);
        default:
            break;
    }
    return condition_result;
}

static int parse_condition_expression(Stream *input, Token *token)
{
    int condition_result = parse_factor(input, token);
    switch (token->type)
    {
        case TOKEN_TYPE_EQUALS:
            DEBUG("==\n");
            return condition_result == parse_condition_expression(input, token);
        case TOKEN_TYPE_LESS_THAN:
            DEBUG("<\n");
            return condition_result < parse_condition_expression(input, token);
        case TOKEN_TYPE_MORE_THAN:
            DEBUG(">\n");
            return condition_result > parse_condition_expression(input, token);
        case TOKEN_TYPE_LESS_THAN_OR_EQUALS:
            DEBUG("<=\n");
            return condition_result <= parse_condition_expression(input, token);
        case TOKEN_TYPE_MORE_THAN_OR_EQUALS:
            DEBUG(">=\n");
            return condition_result >= parse_condition_expression(input, token);
        default:
            break;
    }
    return condition_result;
}

static int parse_ternary(Stream *input, int condition, Token *token)
{
    DEBUG("?\n");
    int lhs = parse_condition_expression(input, token);
    assert (token->type == TOKEN_TYPE_TERNARY_ELSE);
    int rhs = parse_condition_expression(input, token);
    return condition ? lhs : rhs;
}

static int parse_condition(Stream *input, Token *token)
{
    DEBUG("If Condition\n");
    DEBUG_START_SCOPE;

    int condition_result = parse_condition_expression(input, token);
    switch (token->type)
    {
        case TOKEN_TYPE_OR:
            DEBUG("Or\n");
            condition_result = parse_condition(input, token) || condition_result;
            break;
        case TOKEN_TYPE_AND:
            DEBUG("And\n");
            condition_result = parse_condition(input, token) && condition_result;
            break;
        case TOKEN_TYPE_TERNARY:
            condition_result = parse_ternary(input, condition_result, token);
            break;
        default:
            break;
    }
    DEBUG_END_SCOPE;
    DEBUG("End If Condition = %i\n", condition_result);
    return condition_result;
}

int macro_condition_parse(Stream *input)
{
    Token token;
    return parse_condition(input, &token);
}
