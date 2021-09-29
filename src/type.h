#ifndef CEBRA_SIG_H
#define CEBRA_SIG_H

#include "value.h"
#include "table.h"

typedef enum {
    TYPE_ARRAY,
    TYPE_PRIM,
    TYPE_FUN,
    TYPE_CLASS,
    TYPE_IDENTIFIER,
    TYPE_LIST,
    TYPE_MAP,
    TYPE_DECL
} TypeType;

struct Type {
    TypeType type;
    struct Type* next;
    struct Type* opt;
};

struct TypeDecl {
    struct Type base;
};

struct TypeArray {
    struct Type base;
    struct Type** types;
    int count;
    int capacity;
};

struct TypePrim {
    struct Type base;
    ValueType type;
};

struct TypeFun {
    struct Type base;
    struct Type* params;
    struct Type* ret;
};

struct TypeClass {
    struct Type base;
    Token klass;
    struct Table props;
};

struct TypeIdentifier {
    struct Type base;
    Token identifier;
};

struct TypeList {
    struct Type base;
    struct Type* type;
};

struct TypeMap {
    struct Type base;
    struct Type* type;
};


void insert_type(struct Type* type);

struct Type* make_array_type();
void add_type(struct TypeArray* sl, struct Type* type);
struct Type* make_prim_type(ValueType type);
struct Type* make_fun_type(struct Type* params, struct Type* ret);
struct Type* make_class_type(Token klass);
struct Type* make_identifier_type(Token identifier);
struct Type* make_list_type(struct Type* type);
struct Type* make_map_type(struct Type* type);
struct Type* make_decl_type();

bool is_duck(struct TypeClass* sub, struct TypeClass* super);
bool same_type(struct Type* type1, struct Type* type2);
bool type_is_type(struct Type* type, ValueType val);
void print_type(struct Type* type);

struct Type* copy_type(struct Type* type);
void free_type(struct Type* type);


#endif// CEBRA_SIG_H
