#ifndef CEBRA_AST_H
#define CEBRA_AST_H

#include "parser.h"

typedef enum {
    DATA_INT,
    DATA_FLOAT,
} DataType;

typedef enum {
    EXPR_LITERAL,
    EXPR_UNARY,
    EXPR_BINARY,
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

Expr* make_literal(Token name);
Expr* make_unary(Token name, Expr* right);
Expr* make_binary(Token name, Expr* left, Expr* right);

void print_expr(Expr* expr);

DataType type(Expr* expr);


#endif// CEBRA_AST_H
