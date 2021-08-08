#ifndef CEBRA_STATEMENT_LIST_H
#define CEBRA_STATEMENT_LIST_H

#include "memory.h"
#include "ast.h"


typedef struct {
    Expr** stmts;
    int count;
    int capacity;
} StatementList;

void init_statement_list(StatementList* sl);
void free_statement_list(StatementList* sl);
void add_statement(StatementList* sl, Expr* expr);
void print_statement_list(StatementList* sl);

#endif// CEBRA_STATEMENT_LIST_H
