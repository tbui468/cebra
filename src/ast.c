
#include "ast.h"
#include "memory.h"


Expr* make_literal(Token name) {
    Literal* literal = ALLOCATE_NODE(Literal);
    literal->name = name;
    literal->base.type = EXPR_LITERAL;

    return (Expr*)literal;
}


Expr* make_unary(Token name, Expr* right) {
    Unary* unary = ALLOCATE_NODE(Unary);
    unary->name = name;
    unary->right = right;
    unary->base.type = EXPR_UNARY;

    return (Expr*)unary;
}


Expr* make_binary(Token name, Expr* left, Expr* right) {
    Binary* binary = ALLOCATE_NODE(Binary);
    binary->name = name;
    binary->left = left;
    binary->right = right;
    binary->base.type = EXPR_BINARY;

    return (Expr*)binary;
}

Expr* make_print(Token name, Expr* right) {
    Print* print = ALLOCATE_NODE(Print);
    print->name = name;
    print->right = right;
    print->base.type = EXPR_PRINT;

    return (Expr*)print;
}


static void free_binary(Expr* expr) {
    Binary* binary = (Binary*)expr;
    free_expr(binary->left);
    free_expr(binary->right);
    FREE(binary);
}

static void free_literal(Expr* expr) {
    Literal* literal = (Literal*)expr;
    FREE(literal);
}

static void free_unary(Expr* expr) {
    Unary* unary = (Unary*)expr;
    free_expr(unary->right);
    FREE(unary);
}

static void free_print(Expr* expr) {
    Print* print = (Print*)expr;
    free_expr(print->right);
    FREE(print);
}

void free_expr(Expr* expr) {
    switch(expr->type) {
        case EXPR_LITERAL: free_literal(expr); break;
        case EXPR_BINARY: free_binary(expr); break;
        case EXPR_UNARY: free_unary(expr); break;
        case EXPR_PRINT: free_print(expr); break;
    } 
}
