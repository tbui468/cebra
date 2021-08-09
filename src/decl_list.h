#ifndef CEBRA_DECL_LIST_H
#define CEBRA_DECL_LIST_H

#include "memory.h"
#include "ast.h"


typedef struct {
    Expr** stmts;
    int count;
    int capacity;
} DeclList;

void init_decl_list(DeclList* sl);
void free_decl_list(DeclList* sl);
void add_decl(DeclList* sl, Expr* expr);
void print_decl_list(DeclList* sl);

#endif// CEBRA_DECL_LIST_H
