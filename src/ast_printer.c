#include "ast_printer.h"


static void print_binary(Expr* expr) {
    Binary* binary = (Binary*)expr;
    printf("( %.*s ", binary->name.length, binary->name.start);
    print_expr(binary->left);
    print_expr(binary->right);
    printf(")");
}

static void print_literal(Expr* expr) {
    Literal* literal = (Literal*)expr;
    printf(" %.*s ", literal->name.length, literal->name.start);
}

static void print_unary(Expr* expr) {
    Unary* unary = (Unary*)expr;
    printf("( %.*s ", unary->name.length, unary->name.start);
    print_expr(unary->right);
    printf(")");
}

static void print_print(Expr* expr) {
    Print* print = (Print*)expr;
    printf("( %.*s ", print->name.length, print->name.start);
    print_expr(print->right);
    printf(")");
}

static void print_decl_var(Expr* expr) {
    DeclVar* dv = (DeclVar*)expr;
    printf("( DeclVar %.*s ", dv->name.length, dv->name.start);
    print_expr(dv->right);
    printf(")");
}

void print_expr(Expr* expr) {
    switch(expr->type) {
        case EXPR_LITERAL: print_literal(expr); break;
        case EXPR_BINARY: print_binary(expr); break;
        case EXPR_UNARY: print_unary(expr); break;
        case EXPR_PRINT: print_print(expr); break;
        case EXPR_DECL_VAR: print_decl_var(expr); break;
    } 
}

