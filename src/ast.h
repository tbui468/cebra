#ifndef CEBRA_AST_H
#define CEBRA_AST_H

#include "common.h"
#include "token.h"
#include "result_code.h"
#include "decl_list.h"

typedef enum {
    NODE_LITERAL,
    NODE_UNARY,
    NODE_BINARY,
    NODE_PRINT,
    NODE_DECL_VAR,
    NODE_GET_VAR,
    NODE_SET_VAR,
    NODE_BLOCK,
    NODE_IF_ELSE,
    NODE_WHILE,
    NODE_FOR,
} NodeType;

struct Node {
    NodeType type;
};

/*
 * Declarations
 */

typedef struct {
    struct Node base;
    Token name;
    Token type;
    struct Node* right;
} DeclVar;


typedef struct {
    struct Node base;
    Token name;
} GetVar;

typedef struct {
    struct Node base;
    Token name;
    struct Node* right;
    bool decl;
} SetVar;

/*
 * Statements
 */

typedef struct {
    struct Node base;
    Token name;
    struct Node* right;
} Print;

typedef struct {
    struct Node base;
    Token name;
    DeclList decl_list;
} Block;

typedef struct {
    struct Node base;
    Token name;
    struct Node* condition;
    struct Node* then_block;
    struct Node* else_block;
} IfElse;

typedef struct{
    struct Node base;
    Token name;
    struct Node* condition;
    struct Node* then_block;
} While;

typedef struct{
    struct Node base;
    Token name;
    struct Node* initializer;
    struct Node* condition;
    struct Node* update;
    struct Node* then_block;
} For;

/*
 * Expressions
 */

typedef struct {
    struct Node base;
    Token name;
} Literal;

typedef struct {
    struct Node base;
    Token name;
    struct Node* right;
} Unary;

typedef struct {
    struct Node base;
    Token name;
    struct Node* left;
    struct Node* right;
} Binary;



struct Node* make_literal(Token name);
struct Node* make_unary(Token name, struct Node* right);
struct Node* make_binary(Token name, struct Node* left, struct Node* right);
struct Node* make_print(Token name, struct Node* right);
struct Node* make_decl_var(Token name, Token type, struct Node* right);
struct Node* make_get_var(Token name);
struct Node* make_set_var(Token name, struct Node* right, bool decl);
struct Node* make_block(Token name, DeclList dl);
struct Node* make_if_else(Token name, struct Node* condition, struct Node* then_block, struct Node* else_block);
struct Node* make_while(Token name, struct Node* condition, struct Node* then_block);
struct Node* make_for(Token name, struct Node* initializer, struct Node* condition, struct Node* update, struct Node* then_block);

void print_node(struct Node* node);
void free_node(struct Node* node);

#endif// CEBRA_AST_H
