
#include "ast.h"
#include "memory.h"

Typer typer;

static void add_error(Token token, const char* message) {
    TypeError error;
    error.token = token;
    error.message = message;
    typer.errors[typer.error_count] = error;
    typer.error_count++;
}


Expr* make_literal(Token name) {
    Literal* literal = ALLOCATE(Literal, sizeof(Literal));
    literal->name = name;
    literal->base.type = EXPR_LITERAL;

    return (Expr*)literal;
}


Expr* make_unary(Token name, Expr* right) {
    Unary* unary = ALLOCATE(Unary, sizeof(Unary));
    unary->name = name;
    unary->right = right;
    unary->base.type = EXPR_UNARY;

    return (Expr*)unary;
}


Expr* make_binary(Token name, Expr* left, Expr* right) {
    Binary* binary = ALLOCATE(Binary, sizeof(Binary));
    binary->name = name;
    binary->left = left;
    binary->right = right;
    binary->base.type = EXPR_BINARY;

    return (Expr*)binary;
}


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

void print_expr(Expr* expr) {
    switch(expr->type) {
        case EXPR_LITERAL: print_literal(expr); break;
        case EXPR_BINARY: print_binary(expr); break;
        case EXPR_UNARY: print_unary(expr); break;
    } 
}

static DataType type_binary(Expr* expr) {
    Binary* binary = (Binary*)expr;
    DataType left = type(binary->left);
    DataType right = type(binary->right);

    if (left != right) {
        add_error(binary->name, "Types don't match.");
    }

    return left;
}

static DataType type_unary(Expr* expr) {
    Unary* unary = (Unary*)expr;
    DataType right = type(unary->right);
    //if string, make it an error
    return right;
}

static DataType type_literal(Expr* expr) {
    Literal* literal = (Literal*)expr;
    switch(literal->name.type) {
        case TOKEN_INT: return DATA_INT;
        case TOKEN_FLOAT: return DATA_FLOAT;
    }
}

DataType type(Expr* expr) {
    switch(expr->type) {
        case EXPR_LITERAL: return type_literal(expr);
        case EXPR_BINARY: return type_binary(expr);
        case EXPR_UNARY: return type_unary(expr);
    } 
}

ResultCode type_check(Expr* expr) {
    typer.error_count = 0;
    DataType result = type(expr);

    if (typer.error_count > 0) {
        for (int i = 0; i < typer.error_count; i++) {
            printf("[line %d] %s\n", typer.errors[i].token.line, typer.errors[i].message);
        }
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}
