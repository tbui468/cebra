#ifndef CEBRA_LEXER_H
#define CEBRA_LEXER_H

#include "token.h"

typedef struct {
    char* source;
    int start;
    int current;
    int line;
} Lexer;

void init_lexer(char* source);
Token next_token();

#endif// CEBRA_LEXER_H

