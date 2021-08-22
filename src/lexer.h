#ifndef CEBRA_LEXER_H
#define CEBRA_LEXER_H

#include "token.h"

typedef struct {
    const char* source;
    int start;
    int current;
    int line;
} Lexer;

void init_lexer(const char* source);
Token next_token();
char next_char();

#endif// CEBRA_LEXER_H

