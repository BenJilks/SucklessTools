#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main()
{
    lexer_open_file("test.txt");

    Token token;
    for (;;)
    {
        token = lexer_consume(TOKEN_TYPE_NONE);
        if (token.type == TOKEN_TYPE_NONE)
            break;

        printf("Token: %s, Type: %s\n", token.data, lexer_token_type_to_string(token.type));
        free(token.data);
    }

	return 0;
}
