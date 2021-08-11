#ifndef CEBRA_TOKEN_H
#define CEBRA_TOKEN_H

typedef enum {
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_INT_TYPE,
    TOKEN_FLOAT_TYPE,
    TOKEN_STRING_TYPE,
    TOKEN_BOOL_TYPE,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_PRINT,
    TOKEN_IDENTIFIER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_MOD,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COLON,
    TOKEN_EQUAL,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_COMMA,
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
