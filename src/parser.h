#ifndef CEBRA_PARSER_H
#define CEBRA_PARSER_H

#include "common.h"
#include "token.h"
#include "error.h"
#include "result_code.h"
#include "ast.h"
#include "table.h"


typedef struct {
    Token previous;
    Token current;
    Token next;
    Token next_next;
    struct Error* errors;
    int error_count;
    struct Table* globals;
    struct NodeList* statics_nl;
    Token imports[64];
    int import_count;
} Parser;

ResultCode parse(const char* source, struct NodeList* static_nodes, struct NodeList* dynamic_nodes, struct Table* globals, Token* tokens, int* token_count);
ResultCode process_ast(struct NodeList* static_nodes, struct NodeList* dynamic_nodes, struct Table* globals, struct Node* all_nodes, struct NodeList* final_ast);
void print_token(Token token);

#endif// CEBRA_PARSER_H
