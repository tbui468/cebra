#ifndef CEBRA_PARSER_H
#define CEBRA_PARSER_H

#include "common.h"
#include "ast.h"
#include "token.h"
#include "result_code.h"
#include "statement_list.h"

typedef struct {
    Token token;
    const char* message;
} ParseError;


typedef struct {
    char* source;
    int start;
    int current;
    int line;
} Lexer;


typedef struct {
    Token previous;
    Token current;
    ParseError errors[256];
    int error_count;
} Parser;

ResultCode parse(char* source, StatementList* sl);
void print_token(Token token);

#endif// CEBRA_PARSER_H
