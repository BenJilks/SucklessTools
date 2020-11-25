#include "lexer.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>

#define PEEK_QUEUE_SIZE 10

// State
static Token g_peek_queue[PEEK_QUEUE_SIZE];
static int g_peek_queue_start = 0;
static int g_peek_queue_end = 0;
static int g_peek_queue_count = 0;
static Token g_null_token = { NULL, 0, TOKEN_TYPE_NONE, { "null", 0, 0, 0 } };

// Source data
static char *g_source = NULL;
static int g_source_length;
static int g_source_pointer;
static int g_source_line;
static int g_source_column;
static SourceMap *g_source_map;

// Current token state
static char g_c = '\0';
static int g_should_reconsume = 0;

static int find_start_of_line(SourceLine *line)
{
    int index = line->source_index;
    for (;;)
    {
        if (index < 0)
            break;
        if (g_source[index] == '\n')
            break;
        index -= 1;
    }

    return index + 1;
}

void lexer_error(Token *token, const char *message)
{
    // If a token was specified, use its line data, 
    // otherwise use the current position.
    SourceLine line;
    if (!token)
        line = source_map_entry_for_line(g_source_map, g_source_pointer, g_source_line, g_source_column);
    else
        line = token->line;

    // Print start of error message
    fprintf(stderr, "[Error '%s' %i:%i] %s\n",
        line.file_name, line.line_no, line.column_no, message);
    fprintf(stderr, "%5i | ", line.line_no);

    // Print the corresponding line
    int pointer = find_start_of_line(&line);
    while (pointer < g_source_length)
    {
        char c = g_source[pointer++];
        if (c == '\n')
            break;
        fprintf(stderr, "%c", c);
    }
    fprintf(stderr, "\n");

    // Print a marker to show the token position
    for (int i = 0; i < 8 + g_source_column - 4; i++)
        fprintf(stderr, " ");
    fprintf(stderr, "^^^^\n\n");
}

static void reset_lexer_state()
{
    g_source_pointer = 0;
    g_source_line = 1;
    g_source_column = 1;
    g_peek_queue_start = 0;
    g_peek_queue_end = 0;
    g_peek_queue_count = 0;
}

void lexer_open_file(const char *file_path, SourceMap *source_map)
{
    assert (!g_source);

    // Open the file and read its length
    FILE *file = fopen(file_path, "r");
    fseek(file, 0L, SEEK_END);
    g_source_length = ftell(file);
    rewind(file);

    // Read the contents into memory
    g_source = malloc(g_source_length);
    fread(g_source, sizeof(char), g_source_length, file);
    fclose(file);

    // Setup pointers
    g_source_map = source_map;
    reset_lexer_state();
}

void lexer_open_memory(const char *data, int data_len, SourceMap *source_map)
{
    assert (!g_source);
 
    // Setup memory pointers
    g_source = (char*)data;
    g_source_length = data_len;
    g_source_map = source_map;

    // Reset state
    reset_lexer_state();
}

static int is_eof()
{
    return g_source_pointer >= g_source_length;
}

enum State
{
	STATE_INITIAL,
	STATE_IDENTIFIER,
    STATE_INTEGER,
    STATE_FLOAT,
    STATE_STRING,
    STATE_DOT,
    STATE_DOT_DOT,
    STATE_EXCLAMATION,
};

typedef struct Buffer
{
	char *memory;
	int size;
	int pointer;
} Buffer;

static Token make_single_char_token(enum TokenType type)
{
    Token token;
    token.data = g_source + g_source_pointer - 1;
    token.line = source_map_entry_for_line(g_source_map, 
        g_source_pointer - 1, g_source_line, g_source_column);

    token.length = 1;
    token.type = type;
    return token;
}

static enum TokenType check_keyword(Token *token)
{
    if (lexer_compair_token_name(token, "const"))
        return TOKEN_TYPE_CONST;
    if (lexer_compair_token_name(token, "return"))
        return TOKEN_TYPE_RETURN;
    if (lexer_compair_token_name(token, "if"))
        return TOKEN_TYPE_IF;
    if (lexer_compair_token_name(token, "while"))
        return TOKEN_TYPE_WHILE;
    if (lexer_compair_token_name(token, "typedef"))
        return TOKEN_TYPE_TYPEDEF;
    if (lexer_compair_token_name(token, "struct"))
        return TOKEN_TYPE_STRUCT;
    if (lexer_compair_token_name(token, "unsigned"))
        return TOKEN_TYPE_UNSIGNED;
    if (lexer_compair_token_name(token, "enum"))
        return TOKEN_TYPE_ENUM;
    return TOKEN_TYPE_IDENTIFIER;
}

static Token lexer_next()
{
	enum State state = STATE_INITIAL;

    // Create default token state
    Token token;
    token.type = TOKEN_TYPE_NONE;
    token.data = NULL;
    token.length = 0;

    for (;;)
	{
        // If we're not re-consuming, fetch the next token
        if (!g_should_reconsume)
        {
            g_c = is_eof() ? '\0' : g_source[g_source_pointer++];

            // Work out next line/column number
            g_source_column += 1;
            if (g_c == '\n')
            {
                g_source_line += 1;
                g_source_column = 1;
            }
        }

        // Reset re-consume flag
        g_should_reconsume = 0;

		switch (state)
		{
			case STATE_INITIAL:
                if (is_eof())
                    return g_null_token;

                if (isspace(g_c))
					break;

                // Set token source map information
                token.line = source_map_entry_for_line(g_source_map, 
                        g_source_pointer, g_source_line, g_source_column);

                // Identifier
                if (isalpha(g_c))
				{
                    state = STATE_IDENTIFIER;
                    token.data = g_source + g_source_pointer - 1;
                    g_should_reconsume = 1;
					break;
				}

                // Number
                if (isdigit(g_c))
                {
                    state = STATE_INTEGER;
                    token.data = g_source + g_source_pointer - 1;
                    g_should_reconsume = 1;
                    break;
                }

                // String
                if (g_c == '"')
                {
                    state = STATE_STRING;
                    token.data = g_source + g_source_pointer;
                    break;
                }

                // ... Or .
                if (g_c == '.')
                {
                    state = STATE_DOT;
                    token.data = g_source + g_source_pointer - 1;
                    token.length = 1;
                    break;
                }

                // !! Or !
                if (g_c == '!')
                {
                    state = STATE_EXCLAMATION;
                    token.data = g_source + g_source_pointer - 1;
                    token.length = 1;
                    break;
                }

                // Any other single char tokens
                switch (g_c)
                {
                    case '(':
                        return make_single_char_token(TOKEN_TYPE_OPEN_BRACKET);
                    case ')':
                        return make_single_char_token(TOKEN_TYPE_CLOSE_BRACKET);
                    case '{':
                        return make_single_char_token(TOKEN_TYPE_OPEN_SQUIGGLY);
                    case '}':
                        return make_single_char_token(TOKEN_TYPE_CLOSE_SQUIGGLY);
                    case ',':
                        return make_single_char_token(TOKEN_TYPE_COMMA);
                    case ';':
                        return make_single_char_token(TOKEN_TYPE_SEMI);
                    case '+':
                        return make_single_char_token(TOKEN_TYPE_ADD);
                    case '=':
                        return make_single_char_token(TOKEN_TYPE_EQUALS);
                    case '*':
                        return make_single_char_token(TOKEN_TYPE_STAR);
                    case '&':
                        return make_single_char_token(TOKEN_TYPE_AND);
                    case '<':
                        return make_single_char_token(TOKEN_TYPE_LESS_THAN);
                    case '>':
                        return make_single_char_token(TOKEN_TYPE_GREATER_THAN);
                    case '-':
                        return make_single_char_token(TOKEN_TYPE_SUBTRACT);
                    case '[':
                        return make_single_char_token(TOKEN_TYPE_OPEN_SQUARE);
                    case ']':
                        return make_single_char_token(TOKEN_TYPE_CLOSE_SQUARE);
                    case '/':
                        return make_single_char_token(TOKEN_TYPE_FORWARD_SLASH);
                }

                assert (0);
				break;

			case STATE_IDENTIFIER:
                if (!isalnum(g_c) && g_c != '_')
				{
                    token.type = check_keyword(&token);
					state = STATE_INITIAL;
                    g_should_reconsume = 1;
                    return token;
				}

                token.length += 1;
				break;

            case STATE_INTEGER:
                if (g_c == '.')
                {
                    // TODO: Check we only have a single dot
                    state = STATE_FLOAT;
                    token.length += 1;
                    break;
                }

                if (!isdigit(g_c))
                {
                    token.type = TOKEN_TYPE_INTEGER;
                    state = STATE_INITIAL;
                    g_should_reconsume = 1;
                    return token;
                }

                token.length += 1;
                break;

            case STATE_FLOAT:
                if (!isdigit(g_c))
                {
                    token.type = TOKEN_TYPE_FLOAT;
                    state = STATE_INITIAL;
                    g_should_reconsume = 1;
                    return token;
                }

                token.length += 1;
                break;

            case STATE_STRING:
                if (g_c == '"')
                {
                    token.type = TOKEN_TYPE_STRING;
                    state = STATE_INITIAL;
                    return token;
                }

                token.length += 1;
                break;

            case STATE_DOT:
                if (g_c == '.')
                {
                    state = STATE_DOT_DOT;
                    token.length += 1;
                    break;
                }

                state = STATE_INITIAL;
                g_should_reconsume = 1;
                return make_single_char_token(TOKEN_TYPE_DOT);

            case STATE_DOT_DOT:
                if (g_c == '.')
                {
                    state = STATE_INITIAL;
                    token.length += 1;
                    token.type = TOKEN_TYPE_ELLIPSE;
                    return token;
                }

                // Can't have a .. token
                ERROR(NULL, "Unexpected char '%c'", g_c);
                state = STATE_INITIAL;
                g_should_reconsume = 1;
                break;
            case STATE_EXCLAMATION:
                if (g_c == '=')
                {
                    state = STATE_INITIAL;
                    token.length += 1;
                    token.type = TOKEN_TYPE_NOT_EQUALS;
                    return token;
                }

                assert (0);
                break;
        }
	}
}

Token lexer_peek(int count)
{
    // Read and load tokens into the peek queue, until 
    // we get to the one we're looking for.
    while (count >= g_peek_queue_count)
    {
        Token token = lexer_next();

        // If we run out of tokens, return a null one
        if (token.type == TOKEN_TYPE_NONE)
            return token;

        // Add to queue operation
        g_peek_queue[g_peek_queue_end] = token;
        g_peek_queue_end = (g_peek_queue_end + 1) % PEEK_QUEUE_SIZE;
        g_peek_queue_count += 1;
    }

    // Count tokens into peek queue
    return g_peek_queue[(g_peek_queue_start + count) % PEEK_QUEUE_SIZE];
}

Token lexer_consume(enum TokenType expected)
{
    // If the token is the expected one, then consume it.
    // Otherwise, ignore it. If the expected is NONE, 
    // consume anything.
    Token token = lexer_peek(0);
    if (expected != TOKEN_TYPE_NONE && token.type != expected)
        return g_null_token;

    // Take token off peek queue
    g_peek_queue_start = (g_peek_queue_start + 1) % PEEK_QUEUE_SIZE;
    g_peek_queue_count -= 1;
    return token;
}

const char *lexer_token_type_to_string(enum TokenType type)
{
    switch (type)
    {
#define __TOKEN_TYPE(x) case TOKEN_TYPE_##x: return #x;
        ENUMERATE_TOKEN_TYPES
#undef __TOKEN_TYPE
    }

    return "UNKOWN";
}

char *lexer_printable_token_data(Token *token)
{
    static char buffer[1024];

    memcpy(buffer, token->data, token->length);
    buffer[token->length] = '\0';
    return buffer;
}

int lexer_compair_token_name(Token *token, const char *name)
{
    int name_len = strlen(name);
    if (token->length != name_len)
        return 0;

    for (int i = 0; i < name_len; i++)
    {
        if (token->data[i] != name[i])
            return 0;
    }
    return 1;
}

int lexer_compair_token_token(Token *lhs, Token *rhs)
{
    if (lhs->length != rhs->length)
        return 0;

    for (int i = 0; i < lhs->length; i++)
    {
        if (lhs->data[i] != rhs->data[i])
            return 0;
    }
    return 1;
}

void lexer_close()
{
    assert (g_source);
    free(g_source);
}
