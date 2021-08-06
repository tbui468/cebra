#ifndef CEBRA_PARSER_H
#define CEBRA_PARSER_H

#include "common.h"

typedef enum {
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_EOF,
} TokenType;

typedef struct {
    TokenType type;
    char* start;
    int length;
} Token;


typedef struct {
    char* source;
    int start;
    int current;
} Lexer;


typedef struct {
    Token previous;
    Token current;
} Parser;

void parse(char* source);
void print_token(Token token);

#endif// CEBRA_PARSER_H
