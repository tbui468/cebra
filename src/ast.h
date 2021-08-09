#ifndef CEBRA_AST_H
#define CEBRA_AST_H

#include "common.h"
#include "token.h"
#include "result_code.h"

typedef enum {
    EXPR_LITERAL,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_PRINT,
} ExprType;

typedef struct {
    ExprType type;
} Expr;

typedef struct {
    Expr base;
    Token name;
} Literal;

typedef struct {
    Expr base;
    Token name;
    Expr* right;
} Unary;

typedef struct {
    Expr base;
    Token name;
    Expr* left;
    Expr* right;
} Binary;

typedef struct {
    Expr base;
    Token name;
    Expr* right;
} Print;

Expr* make_literal(Token name);
Expr* make_unary(Token name, Expr* right);
Expr* make_binary(Token name, Expr* left, Expr* right);
Expr* make_print(Token name, Expr* right);

void free_expr(Expr* expr);


#endif// CEBRA_AST_H
