#ifndef LEXER_H
#define LEXER_H

#define ENUMERATE_TOKEN_TYPES \
    __TOKEN_TYPE(NONE) \
    __TOKEN_TYPE(IDENTIFIER) \
    __TOKEN_TYPE(INTEGER) \
    __TOKEN_TYPE(FLOAT) \
    __TOKEN_TYPE(STRING) \
	__TOKEN_TYPE(OPEN_BRACKET) \
	__TOKEN_TYPE(CLOSE_BRACKET) \
	__TOKEN_TYPE(OPEN_SQUIGGLY) \
    __TOKEN_TYPE(CLOSE_SQUIGGLY) \
    __TOKEN_TYPE(COMMA) \
    __TOKEN_TYPE(SEMI) \
    __TOKEN_TYPE(ADD) \
    __TOKEN_TYPE(EQUALS) \
    __TOKEN_TYPE(CONST) \
    __TOKEN_TYPE(STAR) \
    __TOKEN_TYPE(RETURN) \
    __TOKEN_TYPE(ELLIPSE) \
    __TOKEN_TYPE(AND) \
    __TOKEN_TYPE(IF) \
    __TOKEN_TYPE(LESS_THAN) \
    __TOKEN_TYPE(SUBTRACT) \
    __TOKEN_TYPE(WHILE) \
    __TOKEN_TYPE(TYPEDEF) \
    __TOKEN_TYPE(STRUCT) \
    __TOKEN_TYPE(DOT)

enum TokenType
{
#define __TOKEN_TYPE(x) TOKEN_TYPE_##x,
ENUMERATE_TOKEN_TYPES
#undef __TOKEN_TYPE
};

typedef struct Token
{
	char *data;
    int length;
	enum TokenType type;
} Token;

void lexer_open_file(const char *file_path);
void lexer_open_memory(const char *data, int data_len);
void lexer_close();

#define ERROR(...) \
{ \
    char buffer[80]; \
    sprintf(buffer, __VA_ARGS__); \
    lexer_error(buffer); \
}

void lexer_error(const char *message);

char *lexer_printable_token_data(Token*);
int lexer_compair_token_name(Token*, const char *name);
int lexer_compair_token_token(Token*, Token*);

Token lexer_consume(enum TokenType);
Token lexer_peek(int count);
const char *lexer_token_type_to_string(enum TokenType);

#endif // LEXER_H
