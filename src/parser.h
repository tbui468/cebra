#ifndef CEBRA_PARSER_H
#define CEBRA_PARSER_H

#include "common.h"
#include "token.h"
#include "result_code.h"
#include "ast.h"
#include "table.h"


typedef struct {
    Token token;
    const char* message;
} ParseError;

typedef struct {
    Token previous;
    Token current;
    Token next;
    Token next_next;
    ParseError errors[256];
    int error_count;
} Parser;

ResultCode parse(const char* source, struct NodeList* nl);
void print_token(Token token);

#endif// CEBRA_PARSER_H
