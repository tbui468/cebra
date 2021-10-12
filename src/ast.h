#ifndef CEBRA_AST_H
#define CEBRA_AST_H

#include "common.h"
#include "token.h"
#include "result_code.h"
#include "value.h"
#include "type.h"

typedef enum {
    NODE_LITERAL,
    NODE_UNARY,
    NODE_BINARY,
    NODE_LOGICAL,
    NODE_DECL_VAR,
    NODE_STRUCT,
    NODE_GET_PROP,
    NODE_SET_PROP,
    NODE_GET_VAR,
    NODE_SET_VAR,
    NODE_GET_ELEMENT,
    NODE_SET_ELEMENT,
    NODE_BLOCK,
    NODE_IF_ELSE,
    NODE_WHILE,
    NODE_FOR,
    NODE_FOR_EACH,
    NODE_FUN,
    NODE_RETURN,
    NODE_CALL,
    NODE_EXPR_STMT,
    NODE_NIL,
    NODE_LIST,
    NODE_ENUM,
    NODE_CAST,
    NODE_CONTAINER,
    NODE_WHEN
} NodeType;

struct Node {
    NodeType type;
    struct Node* next;
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
    struct Type* type;
    struct Node* right;
} DeclVar;

typedef struct {
    struct Node base;
    Token name;
    struct NodeList* parameters;
    struct Type* type;
    struct Node* body;
    bool anonymous;
} DeclFun;

struct DeclStruct {
    struct Node base;
    Token name;
    struct Node* super;
    struct NodeList* decls;
};

struct DeclEnum {
    struct Node base;
    Token name;
    struct Node* identifier;
    struct NodeList* decls;
};

struct DeclContainer {
    struct Node base;
    Token name;
    struct Type* type;
};

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

struct When {
    struct Node base;
    Token name;
    struct NodeList* cases;
};

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
    struct Node* element;
    struct Node* var_list;
    struct Node* then_block;
} ForEach;

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
    struct Type* template_type;
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
} GetElement;

typedef struct {
    struct Node base;
    struct Node* left;
    struct Node* right;
} SetElement;

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

typedef struct {
    struct Node base;
    Token name;
    struct Node* left;
    struct Type* type;
} Cast;


struct Node* make_node_list();
void add_node(struct NodeList* nl, struct Node* node);
struct Node* make_literal(Token name);
struct Node* make_unary(Token name, struct Node* right);
struct Node* make_binary(Token name, struct Node* left, struct Node* right);
struct Node* make_logical(Token name, struct Node* left, struct Node* right);
struct Node* make_decl_var(Token name, struct Type* type, struct Node* right);
struct Node* make_get_var(Token name, struct Type* template_type);
struct Node* make_set_var(struct Node* left, struct Node* right);
struct Node* make_get_element(Token name, struct Node* left, struct Node* idx);
struct Node* make_set_element(struct Node* left, struct Node* right);
struct Node* make_block(Token name, struct NodeList* dl);
struct Node* make_if_else(Token name, struct Node* condition, struct Node* then_block, struct Node* else_block);
struct Node* make_when(Token name, struct NodeList* cases);
struct Node* make_while(Token name, struct Node* condition, struct Node* then_block);
struct Node* make_for(Token name, struct Node* initializer, struct Node* condition, struct Node* update, struct Node* then_block);
struct Node* make_for_each(Token name, struct Node* element, struct Node* var_list, struct Node* then_block);
struct Node* make_decl_fun(Token name, struct NodeList* parameters, struct Type* type, struct Node* body, bool anonymous);
struct Node* make_return(Token name, struct Node* right);
struct Node* make_call(Token name, struct Node* left, struct NodeList* arguments);
struct Node* make_decl_struct(Token name, struct Node* super, struct NodeList* decls);
struct Node* make_expr_stmt(struct Node* expr);
struct Node* make_get_prop(struct Node* inst, Token prop);
struct Node* make_set_prop(struct Node* inst, struct Node* right);
struct Node* make_nil(Token name);
struct Node* make_decl_enum(Token name, struct NodeList* decls);
struct Node* make_cast(Token name, struct Node* left, struct Type* type);
struct Node* make_decl_container(Token name, struct Type* type);

void print_node(struct Node* node);
void free_node(struct Node* node);

#endif// CEBRA_AST_H
