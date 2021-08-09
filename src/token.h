#ifndef CEBRA_TOKEN_H
#define CEBRA_TOKEN_H

typedef enum {
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_PRINT,
    TOKEN_IDENTIFIER,
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
    int line;
    const char* start;
    int length;
} Token;



void print_token(Token token);

#endif// CEBRA_TOKEN_H
