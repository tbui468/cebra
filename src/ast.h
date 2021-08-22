#ifndef CEBRA_AST_H
#define CEBRA_AST_H

#include "common.h"
#include "token.h"
#include "result_code.h"
#include "node_list.h"
#include "value.h"
#include "sig.h"

typedef enum {
    NODE_LITERAL,
    NODE_UNARY,
    NODE_BINARY,
    NODE_LOGICAL,
    NODE_PRINT,
    NODE_DECL_VAR,
    NODE_DECL_CLASS,
    NODE_GET_VAR,
    NODE_SET_VAR,
    NODE_BLOCK,
    NODE_IF_ELSE,
    NODE_WHILE,
    NODE_FOR,
    NODE_DECL_FUN,
    NODE_RETURN,
    NODE_CALL,
    NODE_CASCADE_CALL,
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
    Sig* sig;
    struct Node* right;
} DeclVar;

typedef struct {
    struct Node base;
    Token name;
    NodeList parameters;
    Sig* sig;
    struct Node* body;
} DeclFun;

typedef struct {
    struct Node base;
    Token name;
    NodeList decls;
    Sig* sig;
} DeclClass;

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
    NodeList decl_list;
} Block;

typedef struct {
    struct Node base;
    Token name;
    struct Node* condition;
    struct Node* then_block;
    struct Node* else_block;
} IfElse;

typedef struct {
    struct Node base;
    Token name;
    struct Node* condition;
    struct Node* then_block;
} While;

typedef struct {
    struct Node base;
    Token name;
    struct Node* initializer;
    struct Node* condition;
    struct Node* update;
    struct Node* then_block;
} For;

typedef struct {
    struct Node base;
    Token name;
    struct Node* right;
} Return;

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

typedef struct {
    struct Node base;
    Token name;
    struct Node* left;
    struct Node* right;
} Logical;

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

typedef struct {
    struct Node base;
    Token name;
    NodeList arguments;
} Call;

typedef struct {
    struct Node base;
    Token name;
    struct Node* function;
    NodeList arguments;
} CascadeCall;


struct Node* make_literal(Token name);
struct Node* make_unary(Token name, struct Node* right);
struct Node* make_binary(Token name, struct Node* left, struct Node* right);
struct Node* make_logical(Token name, struct Node* left, struct Node* right);
struct Node* make_print(Token name, struct Node* right);
struct Node* make_decl_var(Token name, Sig* sig, struct Node* right);
struct Node* make_get_var(Token name);
struct Node* make_set_var(Token name, struct Node* right, bool decl);
struct Node* make_block(Token name, NodeList dl);
struct Node* make_if_else(Token name, struct Node* condition, struct Node* then_block, struct Node* else_block);
struct Node* make_while(Token name, struct Node* condition, struct Node* then_block);
struct Node* make_for(Token name, struct Node* initializer, struct Node* condition, struct Node* update, struct Node* then_block);
struct Node* make_decl_fun(Token name, NodeList parameters, Sig* sig, struct Node* body);
struct Node* make_return(Token name, struct Node* right);
struct Node* make_call(Token name, NodeList arguments);
struct Node* make_cascade_call(Token name, struct Node* function, NodeList arguments);
struct Node* make_decl_class(Token name, NodeList decls, Sig* sig);

void print_node(struct Node* node);
void free_node(struct Node* node);

#endif// CEBRA_AST_H
