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

ResultCode parse(const char* source, struct NodeList** nl, struct Table* globals, struct Node** nodes, struct ObjString* script_path);
void print_token(Token token);

#endif// CEBRA_PARSER_H
