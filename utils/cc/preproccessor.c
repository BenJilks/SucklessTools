#include "preproccessor.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#define DEBUG_PRE_PROCCESSOR

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
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_OTHER_CHAR,
    TOKEN_TYPE_MACRO_INCLUDE,
    TOKEN_TYPE_MACRO_DEFINE,
    TOKEN_TYPE_MACRO_UNDEF,
    TOKEN_TYPE_MACRO_IF,
    TOKEN_TYPE_MACRO_ENDIF,
    TOKEN_TYPE_MACRO_ELSE,
    TOKEN_TYPE_MACRO_ELIF,
    TOKEN_TYPE_MACRO_IFDEF,
    TOKEN_TYPE_MACRO_IFNDEF,
    TOKEN_TYPE_MACRO_WARNING,
    TOKEN_TYPE_MACRO_ERROR,
    TOKEN_TYPE_GLOBAL_INCLUDE,
    TOKEN_TYPE_LOCAL_INCLUDE,
    TOKEN_TYPE_DEFINE_VALUE,
    TOKEN_TYPE_MACRO_MESSAGE,
    TOKEN_TYPE_MACRO_CONDITION_NUMBER,
    TOKEN_TYPE_MACRO_CONDITION_NOT,
    TOKEN_TYPE_MACRO_CONDITION_OR,
    TOKEN_TYPE_MACRO_CONDITION_AND,
    TOKEN_TYPE_MACRO_CONDITION_EQUALS,
    TOKEN_TYPE_MACRO_CONDITION_MORE_THAN,
    TOKEN_TYPE_MACRO_CONDITION_LESS_THAN,
    TOKEN_TYPE_MACRO_CONDITION_MORE_THAN_OR_EQUALS,
    TOKEN_TYPE_MACRO_CONDITION_LESS_THAN_OR_EQUALS,
    TOKEN_TYPE_MACRO_CONDITION_SUBTRACT,
    TOKEN_TYPE_MACRO_CONDITION_TERNARY,
    TOKEN_TYPE_MACRO_CONDITION_TERNARY_ELSE,
    TOKEN_TYPE_MACRO_CONDITION_OPEN_BRACKET,
    TOKEN_TYPE_MACRO_CONDITION_CLOSE_BRACKET,
};

enum LexerState
{
    LEXER_STATE_START_OF_LINE,
    LEXER_STATE_IN_LINE,
    LEXER_STATE_SLASH,
    LEXER_STATE_MULTI_LINE_COMMENT,
    LEXER_STATE_MULTI_LINE_COMMENT_STAR,
    LEXER_STATE_SINGLE_LINE_COMMENT,
    LEXER_STATE_IDENTIFIER,
    LEXER_STATE_MACRO_STATEMENT_START,
    LEXER_STATE_MACRO_STATEMENT_NAME,
    LEXER_STATE_MACRO_STATEMENT_INCLUDE,
    LEXER_STATE_MACRO_STATEMENT_INCLUDE_GLOBAL,
    LEXER_STATE_MACRO_STATEMENT_INCLUDE_LOCAL,
    LEXER_STATE_MACRO_STATEMENT_DEFINE,
    LEXER_STATE_MACRO_STATEMENT_DEFINE_VALUE,
    LEXER_STATE_MACRO_STATEMENT_UNDEF,
    LEXER_STATE_MACRO_STATEMENT_MESSAGE,
    LEXER_STATE_MACRO_CONDITION,
    LEXER_STATE_MACRO_CONDITION_NUMBER,
    LEXER_STATE_MACRO_CONDITION_PIPE,
    LEXER_STATE_MACRO_CONDITION_ESCAPE,
    LEXER_STATE_MACRO_CONDITION_AND,
    LEXER_STATE_MACRO_CONDITION_EQUALS,
    LEXER_STATE_MACRO_CONDITION_GREATER_THAN,
    LEXER_STATE_MACRO_CONDITION_LESS_THAN,
};

enum BlockMode
{
    BLOCK_MODE_NORMAL,
    BLOCK_MODE_DISABLED,
};

typedef struct Token
{
    char data[80];
    enum TokenType type;
} Token;

typedef struct Define
{
    char name[80];
    char value[80];
    struct Define *next;
} Define;

enum LexerSource
{
    LEXER_SOURCE_FILE,
    LEXER_SOURCE_MEMORY,
};

typedef struct Lexer
{
    FILE *file;
    const char *memory;
    int memory_pointer, memory_length;
    enum LexerSource source;

    enum LexerState state, return_state;
    char current_char;
    int should_reconsume;
    Token token, peek;
    int has_peek;
} Lexer;

static void parse_block(Lexer *lexer, enum BlockMode mode);
static int parse_condition(Lexer *lexer);

static int g_output_buffer;
static int g_output_length;
static char *g_output;
static Define *g_define_root;

static Lexer lexer_new(const char *file_path)
{
    Lexer lexer;
    lexer.file = fopen(file_path, "r");
    lexer.source = LEXER_SOURCE_FILE;
    lexer.state = LEXER_STATE_START_OF_LINE;
    lexer.should_reconsume = 0;
    lexer.has_peek = 0;
    return lexer;
}

static Lexer lexer_new_from_memory(const char *data, int data_length)
{
    Lexer lexer;
    lexer.memory = data;
    lexer.memory_pointer = 0;
    lexer.memory_length = data_length;
    lexer.source = LEXER_SOURCE_MEMORY;
    lexer.state = LEXER_STATE_START_OF_LINE;
    lexer.should_reconsume = 0;
    lexer.has_peek = 0;
    return lexer;
}

static int next_token(Lexer *lexer)
{
    if (lexer->has_peek)
    {
        lexer->token = lexer->peek;
        lexer->has_peek = 0;
        return 1;
    }

    int buffer_pointer = 0;
    for (;;)
    {
        if (!lexer->should_reconsume)
        {
            switch (lexer->source)
            {
                case LEXER_SOURCE_FILE:
                    lexer->current_char = fgetc(lexer->file);
                    break;
                case LEXER_SOURCE_MEMORY:
                    if (lexer->memory_pointer >= lexer->memory_length)
                        lexer->current_char = EOF;
                    else
                        lexer->current_char = lexer->memory[lexer->memory_pointer++];
                    break;
            }
        }
        lexer->should_reconsume = 0;

        switch (lexer->state)
        {
            case LEXER_STATE_START_OF_LINE:
                if (isspace(lexer->current_char))
                {
                    lexer->token.data[0] = lexer->current_char;
                    lexer->token.data[1] = '\0';
                    lexer->token.type = TOKEN_TYPE_OTHER_CHAR;
                    return 1;
                }
                if (lexer->current_char == '#')
                {
                    lexer->state = LEXER_STATE_MACRO_STATEMENT_START;
                    break;
                }
                lexer->state = LEXER_STATE_IN_LINE;
                lexer->should_reconsume = 1;
                break;

            case LEXER_STATE_IN_LINE:
                if (lexer->current_char == EOF)
                    return 0;
                if (lexer->current_char == '\n')
                {
                    lexer->state = LEXER_STATE_START_OF_LINE;
                    lexer->token.data[0] = lexer->current_char;
                    lexer->token.data[1] = '\0';
                    lexer->token.type = TOKEN_TYPE_OTHER_CHAR;
                    return 1;
                }
                if (isalpha(lexer->current_char) || lexer->current_char == '_' || lexer->current_char == '$')
                {
                    lexer->return_state = LEXER_STATE_IN_LINE;
                    lexer->state = LEXER_STATE_IDENTIFIER;
                    lexer->should_reconsume = 1;
                    break;
                }
                if (lexer->current_char == '/')
                {
                    lexer->state = LEXER_STATE_SLASH;
                    break;
                }
                lexer->token.data[0] = lexer->current_char;
                lexer->token.data[1] = '\0';
                lexer->token.type = TOKEN_TYPE_OTHER_CHAR;
                return 1;

            case LEXER_STATE_SLASH:
                if (lexer->current_char == '/')
                {
                    lexer->state = LEXER_STATE_SINGLE_LINE_COMMENT;
                    break;
                }
                if (lexer->current_char == '*')
                {
                    lexer->state = LEXER_STATE_MULTI_LINE_COMMENT;
                    break;
                }
                lexer->state = LEXER_STATE_IN_LINE;
                lexer->should_reconsume = 1;
                lexer->token.data[0] = '/';
                lexer->token.data[1] = '\0';
                lexer->token.type = TOKEN_TYPE_OTHER_CHAR;
                return 1;

            case LEXER_STATE_MULTI_LINE_COMMENT:
                if (lexer->current_char == '*')
                    lexer->state = LEXER_STATE_MULTI_LINE_COMMENT_STAR;
                break;

            case LEXER_STATE_MULTI_LINE_COMMENT_STAR:
                if (lexer->current_char == '/')
                {
                    lexer->state = LEXER_STATE_IN_LINE;
                    break;
                }
                lexer->state = LEXER_STATE_MULTI_LINE_COMMENT;
                break;

            case LEXER_STATE_SINGLE_LINE_COMMENT:
                if (lexer->current_char == '\n')
                    lexer->state = LEXER_STATE_START_OF_LINE;
                break;

            case LEXER_STATE_IDENTIFIER:
                if (!isalnum(lexer->current_char) && lexer->current_char != '_' && lexer->current_char != '$')
                {
                    lexer->token.data[buffer_pointer] = '\0';
                    lexer->token.type = TOKEN_TYPE_IDENTIFIER;
                    lexer->state = lexer->return_state;
                    lexer->should_reconsume = 1;
                    return 1;
                }
                lexer->token.data[buffer_pointer++] = lexer->current_char;
                break;

            case LEXER_STATE_MACRO_STATEMENT_START:
                if (isspace(lexer->current_char))
                    break;
                lexer->state = LEXER_STATE_MACRO_STATEMENT_NAME;
                lexer->should_reconsume = 1;
                break;

            case LEXER_STATE_MACRO_STATEMENT_NAME:
                if (isspace(lexer->current_char))
                {
                    lexer->token.data[buffer_pointer] = '\0';
                    lexer->should_reconsume = 1;

                    if (!strcmp(lexer->token.data, "include"))
                    {
                        lexer->state = LEXER_STATE_MACRO_STATEMENT_INCLUDE;
                        lexer->token.type = TOKEN_TYPE_MACRO_INCLUDE;
                    }
                    else if (!strcmp(lexer->token.data, "define"))
                    {
                        lexer->state = LEXER_STATE_MACRO_STATEMENT_DEFINE;
                        lexer->token.type = TOKEN_TYPE_MACRO_DEFINE;
                    }
                    else if (!strcmp(lexer->token.data, "undef"))
                    {
                        lexer->state = LEXER_STATE_MACRO_STATEMENT_UNDEF;
                        lexer->token.type = TOKEN_TYPE_MACRO_UNDEF;
                    }
                    else if (!strcmp(lexer->token.data, "if"))
                    {
                        lexer->state = LEXER_STATE_MACRO_CONDITION;
                        lexer->token.type = TOKEN_TYPE_MACRO_IF;
                    }
                    else if (!strcmp(lexer->token.data, "endif"))
                    {
                        lexer->state = LEXER_STATE_IN_LINE;
                        lexer->token.type = TOKEN_TYPE_MACRO_ENDIF;
                    }
                    else if (!strcmp(lexer->token.data, "else"))
                    {
                        lexer->state = LEXER_STATE_IN_LINE;
                        lexer->token.type = TOKEN_TYPE_MACRO_ELSE;
                    }
                    else if (!strcmp(lexer->token.data, "elif"))
                    {
                        lexer->state = LEXER_STATE_MACRO_CONDITION;
                        lexer->token.type = TOKEN_TYPE_MACRO_ELIF;
                    }
                    else if (!strcmp(lexer->token.data, "ifdef"))
                    {
                        lexer->state = LEXER_STATE_MACRO_STATEMENT_UNDEF;
                        lexer->token.type = TOKEN_TYPE_MACRO_IFDEF;
                    }
                    else if (!strcmp(lexer->token.data, "ifndef"))
                    {
                        lexer->state = LEXER_STATE_MACRO_STATEMENT_UNDEF;
                        lexer->token.type = TOKEN_TYPE_MACRO_IFNDEF;
                    }
                    else if (!strcmp(lexer->token.data, "warning"))
                    {
                        lexer->state = LEXER_STATE_MACRO_STATEMENT_MESSAGE;
                        lexer->token.type = TOKEN_TYPE_MACRO_WARNING;
                    }
                    else if (!strcmp(lexer->token.data, "error"))
                    {
                        lexer->state = LEXER_STATE_MACRO_STATEMENT_MESSAGE;
                        lexer->token.type = TOKEN_TYPE_MACRO_ERROR;
                    }
                    else
                        assert (0);
                    return 1;
                }
                lexer->token.data[buffer_pointer++] = lexer->current_char;
                break;

            case LEXER_STATE_MACRO_STATEMENT_MESSAGE:
                if (lexer->current_char == '\n')
                {
                    lexer->token.data[buffer_pointer] = '\0';
                    lexer->token.type = TOKEN_TYPE_MACRO_MESSAGE;
                    lexer->state = LEXER_STATE_IN_LINE;
                    lexer->should_reconsume = 1;
                    return 1;
                }
                lexer->token.data[buffer_pointer++] = lexer->current_char;
                break;

            case LEXER_STATE_MACRO_STATEMENT_INCLUDE:
                if (lexer->current_char == '\n')
                    assert (0);
                if (isspace(lexer->current_char))
                    break;
                if (lexer->current_char == '<')
                    lexer->state = LEXER_STATE_MACRO_STATEMENT_INCLUDE_GLOBAL;
                else if (lexer->current_char == '"')
                    lexer->state = LEXER_STATE_MACRO_STATEMENT_INCLUDE_LOCAL;
                else
                    assert (0);
                break;

            case LEXER_STATE_MACRO_STATEMENT_INCLUDE_GLOBAL:
                if (lexer->current_char == '>')
                {
                    lexer->token.data[buffer_pointer] = '\0';
                    lexer->token.type = TOKEN_TYPE_GLOBAL_INCLUDE;
                    lexer->state = LEXER_STATE_IN_LINE;
                    return 1;
                }
                lexer->token.data[buffer_pointer++] = lexer->current_char;
                break;

            case LEXER_STATE_MACRO_STATEMENT_INCLUDE_LOCAL:
                if (lexer->current_char == '"')
                {
                    lexer->token.data[buffer_pointer] = '\0';
                    lexer->token.type = TOKEN_TYPE_LOCAL_INCLUDE;
                    lexer->state = LEXER_STATE_IN_LINE;
                    return 1;
                }
                lexer->token.data[buffer_pointer++] = lexer->current_char;
                break;

            case LEXER_STATE_MACRO_STATEMENT_DEFINE:
                if (lexer->current_char == '\n')
                    assert (0);
                if (isspace(lexer->current_char))
                    break;

                lexer->state = LEXER_STATE_IDENTIFIER;
                lexer->return_state = LEXER_STATE_MACRO_STATEMENT_DEFINE_VALUE;
                lexer->should_reconsume = 1;
                break;

            case LEXER_STATE_MACRO_STATEMENT_DEFINE_VALUE:
                if (lexer->current_char == '\n')
                {
                    lexer->token.data[buffer_pointer] = '\0';
                    lexer->token.type = TOKEN_TYPE_DEFINE_VALUE;
                    lexer->state = LEXER_STATE_START_OF_LINE;
                    lexer->should_reconsume = 1;
                    return 1;
                }
                lexer->token.data[buffer_pointer++] = lexer->current_char;
                break;

            case LEXER_STATE_MACRO_STATEMENT_UNDEF:
                if (lexer->current_char == '\n')
                    assert (0);
                if (isspace(lexer->current_char))
                    break;
                lexer->return_state = LEXER_STATE_IN_LINE;
                lexer->state = LEXER_STATE_IDENTIFIER;
                lexer->should_reconsume = 1;
                break;

            case LEXER_STATE_MACRO_CONDITION:
                if (lexer->current_char == EOF)
                    return 0;
                if (lexer->current_char == '\n')
                {
                    lexer->state = LEXER_STATE_START_OF_LINE;
                    lexer->should_reconsume = 1;
                    break;
                }
                if (isspace(lexer->current_char))
                    break;
                if (isdigit(lexer->current_char))
                {
                    lexer->state = LEXER_STATE_MACRO_CONDITION_NUMBER;
                    lexer->should_reconsume = 1;
                    break;
                }
                if (isalpha(lexer->current_char) || lexer->current_char == '_')
                {
                    lexer->state = LEXER_STATE_IDENTIFIER;
                    lexer->return_state = LEXER_STATE_MACRO_CONDITION;
                    lexer->should_reconsume = 1;
                    break;
                }
                if (lexer->current_char == '!')
                {
                    lexer->token.data[0] = '!';
                    lexer->token.data[1] = '\0';
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_NOT;
                    return 1;
                }
                if (lexer->current_char == '|')
                {
                    lexer->state = LEXER_STATE_MACRO_CONDITION_PIPE;
                    break;
                }
                if (lexer->current_char == '&')
                {
                    lexer->state = LEXER_STATE_MACRO_CONDITION_AND;
                    break;
                }
                if (lexer->current_char == '\\')
                {
                    lexer->state = LEXER_STATE_MACRO_CONDITION_ESCAPE;
                    break;
                }
                if (lexer->current_char == '=')
                {
                    lexer->state = LEXER_STATE_MACRO_CONDITION_EQUALS;
                    break;
                }
                if (lexer->current_char == '>')
                {
                    lexer->state = LEXER_STATE_MACRO_CONDITION_GREATER_THAN;
                    break;
                }
                if (lexer->current_char == '<')
                {
                    lexer->state = LEXER_STATE_MACRO_CONDITION_LESS_THAN;
                    break;
                }
                if (lexer->current_char == '-')
                {
                    strcpy(lexer->token.data, "-");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_SUBTRACT;
                    return 1;
                }
                if (lexer->current_char == '?')
                {
                    strcpy(lexer->token.data, "?");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_TERNARY;
                    return 1;
                }
                if (lexer->current_char == ':')
                {
                    strcpy(lexer->token.data, ":");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_TERNARY_ELSE;
                    return 1;
                }
                if (lexer->current_char == '(')
                {
                    strcpy(lexer->token.data, "(");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_OPEN_BRACKET;
                    return 1;
                }
                if (lexer->current_char == ')')
                {
                    strcpy(lexer->token.data, ")");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_CLOSE_BRACKET;
                    return 1;
                }
                assert (0);
                break;

            case LEXER_STATE_MACRO_CONDITION_NUMBER:
                if (!isdigit(lexer->current_char) && lexer->current_char != 'L')
                {
                    lexer->token.data[buffer_pointer] = '\0';
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_NUMBER;
                    lexer->state = LEXER_STATE_MACRO_CONDITION;
                    lexer->should_reconsume = 1;
                    return 1;
                }
                lexer->token.data[buffer_pointer++] = lexer->current_char;
                break;

            case LEXER_STATE_MACRO_CONDITION_PIPE:
                if (lexer->current_char == '|')
                {
                    strcpy(lexer->token.data, "||");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_OR;
                    lexer->state = LEXER_STATE_MACRO_CONDITION;
                    return 1;
                }
                assert (0);
                break;

            case LEXER_STATE_MACRO_CONDITION_AND:
                if (lexer->current_char == '&')
                {
                    strcpy(lexer->token.data, "&&");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_AND;
                    lexer->state = LEXER_STATE_MACRO_CONDITION;
                    return 1;
                }
                assert (0);
                break;

            case LEXER_STATE_MACRO_CONDITION_EQUALS:
                if (lexer->current_char == '=')
                {
                    strcpy(lexer->token.data, "==");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_EQUALS;
                    lexer->state = LEXER_STATE_MACRO_CONDITION;
                    return 1;
                }
                assert (0);
                break;

            case LEXER_STATE_MACRO_CONDITION_GREATER_THAN:
                if (lexer->current_char == '=')
                {
                    strcpy(lexer->token.data, ">=");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_MORE_THAN_OR_EQUALS;
                    lexer->state = LEXER_STATE_MACRO_CONDITION;
                    return 1;
                }
                strcpy(lexer->token.data, ">");
                lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_MORE_THAN;
                lexer->state = LEXER_STATE_MACRO_CONDITION;
                lexer->should_reconsume = 1;
                return 1;

            case LEXER_STATE_MACRO_CONDITION_LESS_THAN:
                if (lexer->current_char == '=')
                {
                    strcpy(lexer->token.data, "<=");
                    lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_LESS_THAN_OR_EQUALS;
                    lexer->state = LEXER_STATE_MACRO_CONDITION;
                    return 1;
                }
                strcpy(lexer->token.data, "<");
                lexer->token.type = TOKEN_TYPE_MACRO_CONDITION_LESS_THAN;
                lexer->state = LEXER_STATE_MACRO_CONDITION;
                lexer->should_reconsume = 1;
                return 1;

            case LEXER_STATE_MACRO_CONDITION_ESCAPE:
                if (lexer->current_char == '\n')
                {
                    lexer->state = LEXER_STATE_MACRO_CONDITION;
                    break;
                }
                assert (0);
                break;
        }
    }

    return 0;
}

static void peek(Lexer *lexer)
{
    if (lexer->has_peek)
        return;

    Token temp = lexer->token;
    next_token(lexer);
    lexer->peek = lexer->token;
    lexer->token = temp;
    lexer->has_peek = 1;
}

static void output_string(const char *str)
{
    int str_len = strlen(str);
    while (g_output_length + str_len >= g_output_buffer)
    {
        g_output_buffer += 1024;
        g_output = realloc(g_output, g_output_buffer);
    }

    memcpy(g_output + g_output_length, str, str_len);
    g_output_length += str_len;
}

static void absolute_include_file_path(char *local_file_path, char *out_buffer)
{
    static char *include_locations[] =
    {
        "/usr/local/include/",
        "/usr/include/",
        "/usr/lib/clang/10.0.1/include/"
    };

    strcpy(out_buffer, local_file_path);
    if (access(out_buffer, F_OK) != -1)
        return;

    int include_location_count = (int)sizeof(include_locations) / (int)sizeof(include_locations[0]);
    for (int i = 0; i < include_location_count; i++)
    {
        strcpy(out_buffer, include_locations[i]);
        strcat(out_buffer, local_file_path);
        if (access(out_buffer, F_OK) != -1)
            return;
    }

    fprintf(stderr, "No file found with the name %s\n", local_file_path);
    assert (0);
}

static void parse_include(Lexer *lexer, enum BlockMode mode)
{
    next_token(lexer);
    DEBUG("Include %s\n", lexer->token.data);
    assert (lexer->token.type == TOKEN_TYPE_GLOBAL_INCLUDE || lexer->token.type == TOKEN_TYPE_LOCAL_INCLUDE);
    if (mode == BLOCK_MODE_DISABLED)
        return;

    char absolute_path[80];
    absolute_include_file_path(lexer->token.data, absolute_path);

    DEBUG_START_SCOPE;
    Lexer sub_lexer = lexer_new(absolute_path);
    do
    {
        parse_block(&sub_lexer, mode);
    } while (next_token(&sub_lexer));
    fclose(sub_lexer.file);
}

static void parse_define(Lexer *lexer, enum BlockMode mode)
{
    Define *def = malloc(sizeof(Define));
    next_token(lexer);
    assert (lexer->token.type == TOKEN_TYPE_IDENTIFIER);
    strcpy(def->name, lexer->token.data);

    next_token(lexer);
    assert (lexer->token.type == TOKEN_TYPE_DEFINE_VALUE);
    strcpy(def->value, lexer->token.data);
    if (mode == BLOCK_MODE_DISABLED)
        return;

    def->next = NULL;
    if (g_define_root == NULL)
    {
        g_define_root = def;
        return;
    }

    DEBUG("Define %s = %s\n", def->name, def->value);
    Define *curr = g_define_root;
    while (curr->next)
        curr = curr->next;
    curr->next = def;
}

static Define *find_define(const char *name)
{
    Define *curr = g_define_root;
    while (curr)
    {
        if (!strcmp(curr->name, name))
            return curr;
        curr = curr->next;
    }
    return NULL;
}

static void parse_identifier(Lexer *lexer)
{
    Define *define = find_define(lexer->token.data);
    if (define != NULL)
        output_string(define->value);
    else
        output_string(lexer->token.data);
}

static void parse_undef(Lexer *lexer, enum BlockMode mode)
{
    next_token(lexer);
    assert (lexer->token.type == TOKEN_TYPE_IDENTIFIER);
    if (mode == BLOCK_MODE_DISABLED)
        return;

    Define *last = NULL;
    Define *curr = g_define_root;
    while (curr)
    {
        if (!strcmp(curr->name, lexer->token.data))
        {
            if (last == NULL)
                g_define_root = curr->next;
            else
                last->next = curr->next;
            break;
        }

        last = curr;
        curr = curr->next;
    }
}

static int parse_condition_defined(Lexer *lexer)
{
    char name[80];
    int did_open_bracket = 0;
    next_token(lexer);
    if (lexer->token.type == TOKEN_TYPE_MACRO_CONDITION_OPEN_BRACKET)
    {
        next_token(lexer);
        did_open_bracket = 1;
    }

    assert (lexer->token.type == TOKEN_TYPE_IDENTIFIER);
    strcpy(name, lexer->token.data);
    DEBUG("Defined: %s\n", name);

    if (did_open_bracket)
    {
        next_token(lexer);
        assert (lexer->token.type == TOKEN_TYPE_MACRO_CONDITION_CLOSE_BRACKET);
    }
    return find_define(lexer->token.data) != NULL;
}

static int parse_condition_term(Lexer *lexer)
{
    next_token(lexer);
    switch (lexer->token.type)
    {
        case TOKEN_TYPE_MACRO_CONDITION_NUMBER:
            DEBUG("Number: %s\n", lexer->token.data);
            return atoi(lexer->token.data);
        case TOKEN_TYPE_IDENTIFIER:
        {
            DEBUG("Name: %s\n", lexer->token.data);
            if (!strcmp(lexer->token.data, "defined"))
                return parse_condition_defined(lexer);
            else if (!strcmp(lexer->token.data, "__has_feature"))
                return parse_condition_defined(lexer);

            Define *def = find_define(lexer->token.data);
            if (def == NULL)
                return 0;
            Lexer sub = lexer_new_from_memory(def->value, strlen(def->value));
            sub.state = LEXER_STATE_MACRO_CONDITION;
            return parse_condition(&sub);
        }
        case TOKEN_TYPE_MACRO_CONDITION_NOT:
            DEBUG("Not\n");
            return !parse_condition_term(lexer);
        case TOKEN_TYPE_MACRO_CONDITION_OPEN_BRACKET:
        {
            DEBUG("Sub condition\n");
            DEBUG_START_SCOPE;
            int condition_result = parse_condition(lexer);
            next_token(lexer);
            assert (lexer->token.type == TOKEN_TYPE_MACRO_CONDITION_CLOSE_BRACKET);
            DEBUG_END_SCOPE;
            DEBUG("End sub condition = %i\n", condition_result);
            return condition_result;
        }
        default:
            printf("%s - %i\n", lexer->token.data, lexer->token.type);
            assert (0);
    }
}

static int parse_condition_expression(Lexer *lexer)
{
    int condition_result = parse_condition_term(lexer);

    peek(lexer);
    switch (lexer->peek.type)
    {
        case TOKEN_TYPE_MACRO_CONDITION_EQUALS:
            DEBUG("==\n");
            next_token(lexer);
            return condition_result == parse_condition_expression(lexer);
        case TOKEN_TYPE_MACRO_CONDITION_LESS_THAN:
            DEBUG("<\n");
            next_token(lexer);
            return condition_result < parse_condition_expression(lexer);
        case TOKEN_TYPE_MACRO_CONDITION_MORE_THAN:
            DEBUG(">\n");
            next_token(lexer);
            return condition_result > parse_condition_expression(lexer);
        case TOKEN_TYPE_MACRO_CONDITION_LESS_THAN_OR_EQUALS:
            DEBUG("<=\n");
            next_token(lexer);
            return condition_result <= parse_condition_expression(lexer);
        case TOKEN_TYPE_MACRO_CONDITION_MORE_THAN_OR_EQUALS:
            DEBUG(">=\n");
            next_token(lexer);
            return condition_result >= parse_condition_expression(lexer);
        case TOKEN_TYPE_MACRO_CONDITION_SUBTRACT:
            DEBUG("-\n");
            next_token(lexer);
            return condition_result - parse_condition_expression(lexer);
        default:
            break;
    }
    return condition_result;
}

static int parse_ternary(Lexer *lexer, int condition)
{
    DEBUG("?\n");
    next_token(lexer);

    int lhs = parse_condition_expression(lexer);
    next_token(lexer);
    printf("%s\n", lexer->token.data);
    assert (lexer->token.type == TOKEN_TYPE_MACRO_CONDITION_TERNARY_ELSE);
    int rhs = parse_condition_expression(lexer);
    return condition ? lhs : rhs;
}

static int parse_condition(Lexer *lexer)
{
    DEBUG("If Condition\n");
    DEBUG_START_SCOPE;
    int condition_result = parse_condition_expression(lexer);

    peek(lexer);
    switch (lexer->peek.type)
    {
        case TOKEN_TYPE_MACRO_CONDITION_OR:
            DEBUG("Or\n");
            next_token(lexer);
            condition_result = parse_condition(lexer) || condition_result;
            break;
        case TOKEN_TYPE_MACRO_CONDITION_AND:
            DEBUG("And\n");
            next_token(lexer);
            condition_result = parse_condition(lexer) && condition_result;
            break;
        case TOKEN_TYPE_MACRO_CONDITION_TERNARY:
            condition_result = parse_ternary(lexer, condition_result);
            break;
        default:
            break;
    }
    DEBUG_END_SCOPE;
    DEBUG("End If Condition = %i\n", condition_result);
    return condition_result;
}

static void parse_if_block(Lexer *lexer, enum BlockMode mode, int condition_result)
{
    if (mode == BLOCK_MODE_DISABLED)
        condition_result = 0;

    parse_block(lexer, condition_result ? BLOCK_MODE_NORMAL : BLOCK_MODE_DISABLED);
    while (lexer->token.type == TOKEN_TYPE_MACRO_ELIF)
    {
        int elif_result = parse_condition(lexer);
        if (!condition_result)
            condition_result = elif_result;
        parse_block(lexer, condition_result ? BLOCK_MODE_DISABLED : BLOCK_MODE_NORMAL);
    }
    if (lexer->token.type == TOKEN_TYPE_MACRO_ELSE)
        parse_block(lexer, condition_result ? BLOCK_MODE_DISABLED : BLOCK_MODE_NORMAL);
}

static void parse_if(Lexer *lexer, enum BlockMode mode)
{
    DEBUG("If\n");
    parse_if_block(lexer, mode, parse_condition(lexer));
}

static void parse_ifdef(Lexer *lexer, enum BlockMode mode)
{
    next_token(lexer);
    assert (lexer->token.type == TOKEN_TYPE_IDENTIFIER);
    DEBUG("Ifdef %s\n", lexer->token.data);
    parse_if_block(lexer, mode, find_define(lexer->token.data) != NULL);
}

static void parse_ifndef(Lexer *lexer, enum BlockMode mode)
{
    next_token(lexer);
    assert (lexer->token.type == TOKEN_TYPE_IDENTIFIER);
    DEBUG("Ifndef %s\n", lexer->token.data);
    parse_if_block(lexer, mode, find_define(lexer->token.data) == NULL);
}

static void parse_warning(Lexer *lexer, enum BlockMode mode)
{
    next_token(lexer);
    assert (lexer->token.type == TOKEN_TYPE_MACRO_MESSAGE);
    if (mode != BLOCK_MODE_DISABLED)
        fprintf(stderr, "Warning: %s\n", lexer->token.data);
}

static void parse_error(Lexer *lexer, enum BlockMode mode)
{
    next_token(lexer);
    assert (lexer->token.type == TOKEN_TYPE_MACRO_MESSAGE);
    if (mode != BLOCK_MODE_DISABLED)
        fprintf(stderr, "Error: %s\n", lexer->token.data);
}

static void parse_block(Lexer *lexer, enum BlockMode mode)
{
    DEBUG("Block: %i\n", mode);
    DEBUG_START_SCOPE;
    while (next_token(lexer))
    {
        switch (lexer->token.type)
        {
            case TOKEN_TYPE_OTHER_CHAR:
                if (mode != BLOCK_MODE_DISABLED)
                    output_string(lexer->token.data);
                break;
            case TOKEN_TYPE_IDENTIFIER:
                if (mode != BLOCK_MODE_DISABLED)
                    parse_identifier(lexer);
                break;
            case TOKEN_TYPE_MACRO_INCLUDE:
                parse_include(lexer, mode);
                break;
            case TOKEN_TYPE_MACRO_DEFINE:
                parse_define(lexer, mode);
                break;
            case TOKEN_TYPE_MACRO_UNDEF:
                parse_undef(lexer, mode);
                break;
            case TOKEN_TYPE_MACRO_IF:
                parse_if(lexer, mode);
                break;
            case TOKEN_TYPE_MACRO_IFDEF:
                parse_ifdef(lexer, mode);
                break;
            case TOKEN_TYPE_MACRO_IFNDEF:
                parse_ifndef(lexer, mode);
                break;
            case TOKEN_TYPE_MACRO_WARNING:
                parse_warning(lexer, mode);
                break;
            case TOKEN_TYPE_MACRO_ERROR:
                parse_error(lexer, mode);
                break;
            case TOKEN_TYPE_MACRO_ENDIF:
            case TOKEN_TYPE_MACRO_ELIF:
            case TOKEN_TYPE_MACRO_ELSE:
                // NOTE: End of block
                DEBUG_END_SCOPE;
                DEBUG("EndBlock\n");
                return;
            default:
                printf("%s - %i\n", lexer->token.data, lexer->token.type);
                assert (0);
        }
    }
    DEBUG_END_SCOPE;
    DEBUG("EndBlock\n");
}

const char *pre_proccess_file(const char *file_path, int *out_length)
{
    Lexer lexer = lexer_new(file_path);
    g_output_buffer = 1024;
    g_output_length = 0;
    g_output = malloc(g_output_buffer);
    do
    {
        parse_block(&lexer, BLOCK_MODE_NORMAL);
    } while (next_token(&lexer));

    *out_length = g_output_length;
    fclose(lexer.file);
    return g_output;
}
