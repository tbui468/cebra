#ifndef CEBRA_DECL_LIST_H
#define CEBRA_DECL_LIST_H

#include "memory.h"
#include "ast.h"


typedef struct {
    Expr** decls;
    int count;
    int capacity;
} DeclList;

void init_decl_list(DeclList* dl);
void free_decl_list(DeclList* el);
void add_decl(DeclList* el, Expr* decl);
void print_decl_list(DeclList* el);

#endif// CEBRA_DECL_LIST_H
