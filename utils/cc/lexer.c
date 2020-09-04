#include "lexer.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#define PEEK_QUEUE_SIZE 10

static FILE *g_file = NULL;
static Token g_peek_queue[PEEK_QUEUE_SIZE];
static int g_peek_queue_start = 0;
static int g_peek_queue_end = 0;
static int g_peek_queue_count = 0;
static Token g_null_token = { NULL, TOKEN_TYPE_NONE };

char g_c = '\0';
int g_should_reconsume = 0;
int g_is_eof = 1;

void lexer_open_file(const char *file_path)
{
	assert (g_file == NULL);

	g_file = fopen(file_path, "r");
    g_peek_queue_start = 0;
    g_peek_queue_end = 0;
    g_peek_queue_count = 0;
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

static Buffer new_buffer()
{
	Buffer buffer;
    buffer.memory = NULL;
    buffer.size = 0;
	buffer.pointer = 0;
	return buffer;
}

static void append_buffer(Buffer *buffer, char c)
{
    if (buffer->memory == NULL)
    {
        buffer->memory = malloc(80);
        buffer->size = 80;
    }

	while (buffer->pointer >= buffer->size)
	{
		buffer->size += 80;
		buffer->memory = realloc(buffer->memory, buffer->size);
	}
	buffer->memory[buffer->pointer++] = c;
}

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
	Buffer buffer = new_buffer();
	for (;;)
	{
        if (!g_should_reconsume)
            g_is_eof = (fread(&g_c, 1, 1, g_file) == 0);
        g_should_reconsume = 0;

		switch (state)
		{
			case STATE_INITIAL:
                if (g_is_eof)
                    return g_null_token;

                if (isspace(g_c))
					break;

                if (isalpha(g_c))
				{
                    state = STATE_IDENTIFIER;
                    g_should_reconsume = 1;
					break;
				}

                if (isdigit(g_c))
                {
                    state = STATE_INTEGER;
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
                }

                assert (0);
				break;

			case STATE_IDENTIFIER:
                if (!isalnum(g_c) && g_c != '_')
				{
                    Token token = { buffer.memory, TOKEN_TYPE_IDENTIFIER };
					state = STATE_INITIAL;
                    g_should_reconsume = 1;
                    return token;
				}

                append_buffer(&buffer, g_c);
				break;

            case STATE_INTEGER:
                if (!isdigit(g_c))
                {
                    Token token = { buffer.memory, TOKEN_TYPE_INTEGER };
                    state = STATE_INITIAL;
                    g_should_reconsume = 1;
                    return token;
                }

                append_buffer(&buffer, g_c);
                break;
		}
	}
}

Token lexer_peek(int count)
{
    while (count >= g_peek_queue_count)
    {
        g_peek_queue[g_peek_queue_end] = lexer_next();
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

    g_peek_queue_start += 1;
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
