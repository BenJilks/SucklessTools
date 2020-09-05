#include "lexer.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>

#define PEEK_QUEUE_SIZE 10

static Token g_peek_queue[PEEK_QUEUE_SIZE];
static int g_peek_queue_start = 0;
static int g_peek_queue_end = 0;
static int g_peek_queue_count = 0;
static Token g_null_token = { NULL, 0, TOKEN_TYPE_NONE };

static char *g_source = NULL;
static int g_source_length;
static int g_source_pointer;

static char g_c = '\0';
static int g_should_reconsume = 0;

void lexer_open_file(const char *file_path)
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
    g_source_pointer = 0;
    g_peek_queue_start = 0;
    g_peek_queue_end = 0;
    g_peek_queue_count = 0;
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
};

typedef struct Buffer
{
	char *memory;
	int size;
	int pointer;
} Buffer;

static Token make_single_char_token(char c, enum TokenType type)
{
    Token token;
    token.data = malloc(2);
    token.data[0] = c;
    token.data[1] = '\0';
    token.type = type;
    return token;
}

static Token lexer_next()
{
	enum State state = STATE_INITIAL;

    Token token;
    token.type = TOKEN_TYPE_NONE;
    token.data = NULL;
    token.length = 0;
    for (;;)
	{
        if (!g_should_reconsume)
        {
            g_c = is_eof() ? '\0' : g_source[g_source_pointer++];
        }
        g_should_reconsume = 0;

		switch (state)
		{
			case STATE_INITIAL:
                if (is_eof())
                    return g_null_token;

                if (isspace(g_c))
					break;

                if (isalpha(g_c))
				{
                    state = STATE_IDENTIFIER;
                    token.data = g_source + g_source_pointer - 1;
                    g_should_reconsume = 1;
					break;
				}

                if (isdigit(g_c))
                {
                    state = STATE_INTEGER;
                    token.data = g_source + g_source_pointer - 1;
                    g_should_reconsume = 1;
                    break;
                }

                switch (g_c)
                {
                    case '(':
                        return make_single_char_token(g_c, TOKEN_TYPE_OPEN_BRACKET);
                    case ')':
                        return make_single_char_token(g_c, TOKEN_TYPE_CLOSE_BRACKET);
                    case '{':
                        return make_single_char_token(g_c, TOKEN_TYPE_OPEN_SQUIGGLY);
                    case '}':
                        return make_single_char_token(g_c, TOKEN_TYPE_CLOSE_SQUIGGLY);
                    case ',':
                        return make_single_char_token(g_c, TOKEN_TYPE_COMMA);
                    case ';':
                        return make_single_char_token(g_c, TOKEN_TYPE_SEMI);
                }

                assert (0);
				break;

			case STATE_IDENTIFIER:
                if (!isalnum(g_c) && g_c != '_')
				{
                    token.type = TOKEN_TYPE_IDENTIFIER;
					state = STATE_INITIAL;
                    g_should_reconsume = 1;
                    return token;
				}

                token.length += 1;
				break;

            case STATE_INTEGER:
                if (!isdigit(g_c))
                {
                    token.type = TOKEN_TYPE_INTEGER;
                    state = STATE_INITIAL;
                    g_should_reconsume = 1;
                    return token;
                }

                token.length += 1;
                break;
		}
	}
}

Token lexer_peek(int count)
{
    while (count >= g_peek_queue_count)
    {
        Token token = lexer_next();
        if (token.type == TOKEN_TYPE_NONE)
            return token;

        g_peek_queue[g_peek_queue_end] = token;
        g_peek_queue_end = (g_peek_queue_end + 1) % PEEK_QUEUE_SIZE;
        g_peek_queue_count += 1;
    }
    return g_peek_queue[(g_peek_queue_start + count) % PEEK_QUEUE_SIZE];
}

Token lexer_consume(enum TokenType type)
{
    Token token = lexer_peek(0);
    if (type != TOKEN_TYPE_NONE && token.type != type)
        return g_null_token;

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

void lexer_close()
{
    assert (g_source);
    free(g_source);
}
