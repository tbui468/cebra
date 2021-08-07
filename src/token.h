#ifndef CEBRA_TOKEN_H
#define CEBRA_TOKEN_H

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
    int line;
    char* start;
    int length;
} Token;


#endif// CEBRA_TOKEN_H
