#ifndef CEBRA_TOKEN_H
#define CEBRA_TOKEN_H

#include <stdbool.h>

typedef enum {
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_INT_TYPE,
    TOKEN_FLOAT_TYPE,
    TOKEN_STRING_TYPE,
    TOKEN_BOOL_TYPE,
    TOKEN_FILE_TYPE,
    TOKEN_NIL_TYPE,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
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
    TOKEN_COLON_COLON,
    TOKEN_COLON_EQUAL,
    TOKEN_RIGHT_ARROW,
    TOKEN_EQUAL,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_COMMA,
    TOKEN_STRUCT,
    TOKEN_DOT,
    TOKEN_DUMMY,
    TOKEN_LIST,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_IN,
    TOKEN_MAP,
    TOKEN_NIL,
    TOKEN_FOR_EACH,
    TOKEN_ENUM,
    TOKEN_AS,
    TOKEN_WHEN,
    TOKEN_IS,
    TOKEN_UNDER_SCORE,
    TOKEN_IMPORT,
    TOKEN_EOF,
} TokenType;

typedef struct {
    TokenType type;
    int line;
    const char* start;
    int length;
} Token;



void print_token(Token token);
Token make_dummy_token();
Token make_artificial_token(const char* name);
Token make_token(TokenType type, int line, const char* start, int length);
Token copy_token(Token token);
bool same_token_literal(Token t1, Token t2);

#endif// CEBRA_TOKEN_H
