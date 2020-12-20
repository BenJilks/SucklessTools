#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include "symbol.h"
#include "expression.h"

Token match(enum TokenType, const char *name);
void parse();
void parser_destroy();

#endif // PARSER_H

