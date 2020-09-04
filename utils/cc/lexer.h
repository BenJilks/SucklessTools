#ifndef LEXER_H
#define LEXER_H

#define ENUMERATE_TOKEN_TYPES \
    __TOKEN_TYPE(NONE) \
    __TOKEN_TYPE(IDENTIFIER) \
    __TOKEN_TYPE(INTEGER) \
	__TOKEN_TYPE(OPEN_BRACKET) \
	__TOKEN_TYPE(CLOSE_BRACKET) \
	__TOKEN_TYPE(OPEN_SQUIGGLY) \
	__TOKEN_TYPE(CLOSE_SQUIGGLY)

enum TokenType
{
#define __TOKEN_TYPE(x) TOKEN_TYPE_##x,
ENUMERATE_TOKEN_TYPES
#undef __TOKEN_TYPE
};

typedef struct Token
{
	char *data;
	enum TokenType type;
} Token;

void lexer_open_file(const char *file_path);
void lexer_close();

Token lexer_consume(enum TokenType);
Token lexer_peek(int count);
const char *lexer_token_type_to_string(enum TokenType);

#endif // LEXER_H
