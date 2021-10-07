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
    struct Table* globals;
    struct NodeList* first_pass_nl;
} Parser;

ResultCode parse(const char* source, struct NodeList* first_pass_list, struct NodeList* nl, struct Table* globals);
void print_token(Token token);

#endif// CEBRA_PARSER_H
