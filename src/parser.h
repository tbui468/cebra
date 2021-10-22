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
    struct NodeList* resolve_id_list;
    Token imports[256];
    int import_count;
} Parser;

ResultCode parse(const char* source, struct NodeList** nl, struct Table* globals, struct Node** nodes, struct ObjString* script_path);
void print_token(Token token);

#endif// CEBRA_PARSER_H
