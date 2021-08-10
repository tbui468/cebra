#include "ast.h"
#include "memory.h"


/*
 * Basic
 */

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

/*
 * Variables
 */

Expr* make_decl_var(Token name, Token type, Expr* right) {
    DeclVar* decl_var = ALLOCATE_NODE(DeclVar);
    decl_var->name = name;
    decl_var->type = type;
    decl_var->right = right;
    decl_var->base.type = EXPR_DECL_VAR;

    return (Expr*)decl_var;
}

Expr* make_get_var(Token name) {
    GetVar* get_var = ALLOCATE_NODE(GetVar);
    get_var->name = name;
    get_var->base.type = EXPR_GET_VAR;

    return (Expr*)get_var;
}

Expr* make_set_var(Token name, Expr* right, bool decl) {
    SetVar* set_var = ALLOCATE_NODE(SetVar);
    set_var->name = name;
    set_var->right = right;
    set_var->decl = decl;
    set_var->base.type = EXPR_SET_VAR;

    return (Expr*)set_var;
}

/*
 * Other
 */

Expr* make_print(Token name, Expr* right) {
    Print* print = ALLOCATE_NODE(Print);
    print->name = name;
    print->right = right;
    print->base.type = EXPR_PRINT;

    return (Expr*)print;
}

/*
 * Utility
 */

void print_expr(Expr* expr) {
    switch(expr->type) {
        case EXPR_LITERAL: {
            Literal* literal = (Literal*)expr;
            printf(" %.*s ", literal->name.length, literal->name.start);
            break;
        }
        case EXPR_BINARY: {
            Binary* binary = (Binary*)expr;
            printf("( %.*s ", binary->name.length, binary->name.start);
            print_expr(binary->left);
            print_expr(binary->right);
            printf(")");
            break;
        }
        case EXPR_UNARY: {
            Unary* unary = (Unary*)expr;
            printf("( %.*s ", unary->name.length, unary->name.start);
            print_expr(unary->right);
            printf(")");
            break;
        }
        case EXPR_DECL_VAR: {
            DeclVar* dv = (DeclVar*)expr;
            printf("( DeclVar %.*s ", dv->name.length, dv->name.start);
            print_expr(dv->right);
            printf(")");
            break;
        }
        case EXPR_GET_VAR: {
            //TODO: fill this in
            printf("GetVar");
            break;
        }
        case EXPR_SET_VAR: {
            //TODO: fill this in
            printf("SetVar");
            break;
        }
        case EXPR_PRINT: {
            Print* print = (Print*)expr;
            printf("( %.*s ", print->name.length, print->name.start);
            print_expr(print->right);
            printf(")");
            break;
        }
    } 
}


void free_expr(Expr* expr) {
    switch(expr->type) {
        case EXPR_LITERAL: {
            Literal* literal = (Literal*)expr;
            FREE(literal);
            break;
        }
        case EXPR_BINARY: {
            Binary* binary = (Binary*)expr;
            free_expr(binary->left);
            free_expr(binary->right);
            FREE(binary);
            break;
        }
        case EXPR_UNARY: {
            Unary* unary = (Unary*)expr;
            free_expr(unary->right);
            FREE(unary);
            break;
        }
        case EXPR_DECL_VAR: {
            DeclVar* decl_var = (DeclVar*)expr;
            free_expr(decl_var->right);
            FREE(decl_var);
            break;
        }
        case EXPR_GET_VAR: {
            GetVar* gv = (GetVar*)expr;
            FREE(gv);
            break;
        }
        case EXPR_SET_VAR: {
            SetVar* sv = (SetVar*)expr;
            free_expr(sv->right);
            FREE(sv);
            break;
        }
        case EXPR_PRINT: {
            Print* print = (Print*)expr;
            free_expr(print->right);
            FREE(print);
            break;
        }
    } 
}

