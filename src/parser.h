#ifndef CEBRA_PARSER_H
#define CEBRA_PARSER_H

#include "common.h"
#include "token.h"
#include "result_code.h"
#include "ast.h"
#include "table.h"


struct Error {
    Token token;
    const char* message;
};

typedef struct {
    Token previous;
    Token current;
    Token next;
    Token next_next;
    struct Error errors[256];
    int error_count;
} Parser;

ResultCode parse(const char* source, struct NodeList* nl);
void print_token(Token token);

#endif// CEBRA_PARSER_H
