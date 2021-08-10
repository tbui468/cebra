#include "ast_typer.h"
#include "ast.h"
#include "token.h"

Typer typer;


static DataType type(struct Node* expr);

static DataType data_type_from_token_type(TokenType type) {
    switch(type) {
        case TOKEN_INT_TYPE: return DATA_INT;
        case TOKEN_FLOAT_TYPE: return DATA_FLOAT;
        case TOKEN_STRING_TYPE: return DATA_STRING;
    }
}

static void add_error(Token token, const char* message) {
    TypeError error;
    error.token = token;
    error.message = message;
    typer.errors[typer.error_count] = error;
    typer.error_count++;
}

static DataType type_binary(struct Node* expr) {
    Binary* binary = (Binary*)expr;
    DataType left = type(binary->left);
    DataType right = type(binary->right);

    if (left != right) {
        add_error(binary->name, "Types don't match.");
    }

    return left;
}

static DataType type_unary(struct Node* expr) {
    Unary* unary = (Unary*)expr;
    DataType right = type(unary->right);
    return right;
}

static DataType type_literal(struct Node* expr) {
    Literal* literal = (Literal*)expr;
    switch(literal->name.type) {
        case TOKEN_INT: return DATA_INT;
        case TOKEN_FLOAT: return DATA_FLOAT;
        case TOKEN_STRING: return DATA_STRING;
    }
}

static DataType type_print(struct Node* expr) {
    Print* print = (Print*)expr;
    DataType right = type(print->right);
    return right;
}

static DataType type_decl_var(struct Node* expr) {
    DeclVar* dv = (DeclVar*)expr;
    DataType left = data_type_from_token_type(dv->type.type);
    DataType right = type(dv->right);
    if (left != right) {
        add_error(dv->name, "Declaration type must match assignment type.");
    }
    return right;
}

static DataType type(struct Node* expr) {
    switch(expr->type) {
        case NODE_LITERAL: return type_literal(expr);
        case NODE_BINARY: return type_binary(expr);
        case NODE_UNARY: return type_unary(expr);
        case NODE_PRINT: return type_print(expr);
        case NODE_DECL_VAR: return type_decl_var(expr);
    } 
}

ResultCode type_check(DeclList* dl) {
    typer.error_count = 0;
    for (int i = 0; i < dl->count; i++) {
        type(dl->decls[i]); //discarding result for now
    }

    if (typer.error_count > 0) {
        for (int i = 0; i < typer.error_count; i++) {
            printf("[line %d] %s\n", typer.errors[i].token.line, typer.errors[i].message);
        }
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}
