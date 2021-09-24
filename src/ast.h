#ifndef CEBRA_AST_H
#define CEBRA_AST_H

#include "common.h"
#include "token.h"
#include "result_code.h"
#include "value.h"
#include "sig.h"

typedef enum {
    NODE_LITERAL,
    NODE_UNARY,
    NODE_BINARY,
    NODE_LOGICAL,
    NODE_DECL_VAR,
    NODE_CLASS,
    NODE_GET_PROP,
    NODE_SET_PROP,
    NODE_GET_VAR,
    NODE_SET_VAR,
    NODE_GET_IDX,
    NODE_SET_IDX,
    NODE_BLOCK,
    NODE_IF_ELSE,
    NODE_WHILE,
    NODE_FOR,
    NODE_FUN,
    NODE_RETURN,
    NODE_CALL,
    NODE_EXPR_STMT,
    NODE_NIL,
    NODE_LIST
} NodeType;

struct Node {
    NodeType type;
};

struct NodeList {
    struct Node base;
    struct Node** nodes;
    int count;
    int capacity;
};

/*
 * Declarations
 */

typedef struct {
    struct Node base;
    Token name;
    struct Sig* sig;
    struct Node* right;
} DeclVar;

typedef struct {
    struct Node base;
    Token name;
    struct NodeList* parameters;
    struct Sig* sig;
    struct Node* body;
} DeclFun;

typedef struct {
    struct Node base;
    Token name;
    struct Node* super;
    struct NodeList* decls;
//    struct Sig* sig;
} DeclClass;

/*
 * Statements
 */

typedef struct {
    struct Node base;
    struct Node* expr;
} ExprStmt;


typedef struct {
    struct Node base;
    Token name;
    struct NodeList* decl_list;
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
    struct Node* inst;
    Token prop;
} GetProp;

typedef struct {
    struct Node base;
    struct Node* inst;
    struct Node* right;
} SetProp;

typedef struct {
    struct Node base;
    Token name;
    struct Sig* template_type;
} GetVar;

typedef struct {
    struct Node base;
    struct Node* left;
    struct Node* right;
} SetVar;

typedef struct {
    struct Node base;
    Token name;
    struct Node* left;
    struct Node* idx;
} GetIdx;

typedef struct {
    struct Node base;
    struct Node* left;
    struct Node* right;
} SetIdx;

typedef struct {
    struct Node base;
    Token name;
    struct Node* left;
    struct NodeList* arguments;
} Call;

typedef struct {
    struct Node base;
    Token name;
} Nil;


struct Node* make_node_list();
void add_node(struct NodeList* nl, struct Node* node);
struct Node* make_literal(Token name);
struct Node* make_unary(Token name, struct Node* right);
struct Node* make_binary(Token name, struct Node* left, struct Node* right);
struct Node* make_logical(Token name, struct Node* left, struct Node* right);
struct Node* make_decl_var(Token name, struct Sig* sig, struct Node* right);
struct Node* make_get_var(Token name, struct Sig* template_type);
struct Node* make_set_var(struct Node* left, struct Node* right);
struct Node* make_get_idx(Token name, struct Node* left, struct Node* idx);
struct Node* make_set_idx(struct Node* left, struct Node* right);
struct Node* make_block(Token name, struct NodeList* dl);
struct Node* make_if_else(Token name, struct Node* condition, struct Node* then_block, struct Node* else_block);
struct Node* make_while(Token name, struct Node* condition, struct Node* then_block);
struct Node* make_for(Token name, struct Node* initializer, struct Node* condition, struct Node* update, struct Node* then_block);
struct Node* make_decl_fun(Token name, struct NodeList* parameters, struct Sig* sig, struct Node* body);
struct Node* make_return(Token name, struct Node* right);
struct Node* make_call(Token name, struct Node* left, struct NodeList* arguments);
struct Node* make_decl_class(Token name, struct Node* super, struct NodeList* decls);
struct Node* make_expr_stmt(struct Node* expr);
struct Node* make_get_prop(struct Node* inst, Token prop);
struct Node* make_set_prop(struct Node* inst, struct Node* right);
struct Node* make_nil(Token name);

void print_node(struct Node* node);
void free_node(struct Node* node);

#endif// CEBRA_AST_H
