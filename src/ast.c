#include "ast.h"
#include "memory.h"


/*
 * Declarations
 */

struct Node* make_decl_var(Token name, Token type, struct Node* right) {
    DeclVar* decl_var = ALLOCATE_NODE(DeclVar);
    decl_var->name = name;
    decl_var->type = type;
    decl_var->right = right;
    decl_var->base.type = NODE_DECL_VAR;

    return (struct Node*)decl_var;
}

struct Node* make_get_var(Token name) {
    GetVar* get_var = ALLOCATE_NODE(GetVar);
    get_var->name = name;
    get_var->base.type = NODE_GET_VAR;

    return (struct Node*)get_var;
}

struct Node* make_set_var(Token name, struct Node* right, bool decl) {
    SetVar* set_var = ALLOCATE_NODE(SetVar);
    set_var->name = name;
    set_var->right = right;
    set_var->decl = decl;
    set_var->base.type = NODE_SET_VAR;

    return (struct Node*)set_var;
}

/*
 * Statements
 */

struct Node* make_print(Token name, struct Node* right) {
    Print* print = ALLOCATE_NODE(Print);
    print->name = name;
    print->right = right;
    print->base.type = NODE_PRINT;

    return (struct Node*)print;
}

struct Node* make_block(Token name, DeclList dl) {
    Block* block = ALLOCATE_NODE(Block);
    block->name = name;
    block->decl_list = dl;
    block->base.type = NODE_BLOCK;

    return (struct Node*)block;
}

struct Node* make_if_else(Token name, struct Node* condition, 
                          struct Node* then_block, struct Node* else_block) {
    IfElse* ie = ALLOCATE_NODE(IfElse);
    ie->name = name;
    ie->condition = condition;
    ie->then_block = then_block;
    ie->else_block = else_block; 
    ie->base.type = NODE_IF_ELSE;

    return (struct Node*)ie;
}

/*
 * Expressions
 */

struct Node* make_literal(Token name) {
    Literal* literal = ALLOCATE_NODE(Literal);
    literal->name = name;
    literal->base.type = NODE_LITERAL;

    return (struct Node*)literal;
}

struct Node* make_unary(Token name, struct Node* right) {
    Unary* unary = ALLOCATE_NODE(Unary);
    unary->name = name;
    unary->right = right;
    unary->base.type = NODE_UNARY;

    return (struct Node*)unary;
}

struct Node* make_binary(Token name, struct Node* left, struct Node* right) {
    Binary* binary = ALLOCATE_NODE(Binary);
    binary->name = name;
    binary->left = left;
    binary->right = right;
    binary->base.type = NODE_BINARY;

    return (struct Node*)binary;
}


/*
 * Utility
 */

void print_node(struct Node* node) {
    switch(node->type) {
        //Declarations
        case NODE_DECL_VAR: {
            DeclVar* dv = (DeclVar*)node;
            printf("( DeclVar %.*s ", dv->name.length, dv->name.start);
            print_node(dv->right);
            printf(")");
            break;
        }
        //Statements
        case NODE_PRINT: {
            Print* print = (Print*)node;
            printf("( %.*s ", print->name.length, print->name.start);
            print_node(print->right);
            printf(")");
            break;
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            printf("( Block");
            print_decl_list(&block->decl_list);
            break;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            printf("( If ");
            print_node(ie->condition);
            printf(" then ");
            print_node(ie->then_block);
            if (ie->else_block != NULL) {
                printf(" else ");
                print_node(ie->else_block);
            }
            printf(" )");
            break;
        }
        //struct Expressions
        case NODE_LITERAL: {
            Literal* literal = (Literal*)node;
            printf(" %.*s ", literal->name.length, literal->name.start);
            break;
        }
        case NODE_BINARY: {
            Binary* binary = (Binary*)node;
            printf("( %.*s ", binary->name.length, binary->name.start);
            print_node(binary->left);
            print_node(binary->right);
            printf(")");
            break;
        }
        case NODE_UNARY: {
            Unary* unary = (Unary*)node;
            printf("( %.*s ", unary->name.length, unary->name.start);
            print_node(unary->right);
            printf(")");
            break;
        }
        case NODE_GET_VAR: {
            //TODO: fill this in
            printf("GetVar");
            break;
        }
        case NODE_SET_VAR: {
            //TODO: fill this in
            printf("SetVar");
            break;
        }
    } 
}


void free_node(struct Node* node) {
    switch(node->type) {
        //Declarations
        case NODE_DECL_VAR: {
            DeclVar* decl_var = (DeclVar*)node;
            free_node(decl_var->right);
            FREE(decl_var);
            break;
        }
        //Statements
        case NODE_PRINT: {
            Print* print = (Print*)node;
            free_node(print->right);
            FREE(print);
            break;
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            free_decl_list(&block->decl_list);
            FREE(block);
            break;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            free_node(ie->condition);
            free_node(ie->then_block);
            if (ie->else_block != NULL) {
                free_node(ie->else_block);
            }
            FREE(ie);
            break;
        }
        //struct Expressions
        case NODE_LITERAL: {
            Literal* literal = (Literal*)node;
            FREE(literal);
            break;
        }
        case NODE_BINARY: {
            Binary* binary = (Binary*)node;
            free_node(binary->left);
            free_node(binary->right);
            FREE(binary);
            break;
        }
        case NODE_UNARY: {
            Unary* unary = (Unary*)node;
            free_node(unary->right);
            FREE(unary);
            break;
        }
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;
            FREE(gv);
            break;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            free_node(sv->right);
            FREE(sv);
            break;
        }
    } 
}

