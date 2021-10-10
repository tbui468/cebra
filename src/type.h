#ifndef CEBRA_SIG_H
#define CEBRA_SIG_H

#include "value.h"
#include "table.h"

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_NIL,
    TYPE_ARRAY,
    TYPE_FUN,
    TYPE_STRUCT,
    TYPE_IDENTIFIER,
    TYPE_LIST,
    TYPE_MAP,
    TYPE_INFER,
    TYPE_ENUM
} TypeType;

struct Type {
    TypeType type;
    struct Type* next;
    struct Type* opt;
};

struct TypeInfer {
    struct Type base;
};

struct TypeArray {
    struct Type base;
    struct Type** types;
    int count;
    int capacity;
};

struct TypeInt {
    struct Type base;
};

struct TypeFloat {
    struct Type base;
};

struct TypeBool {
    struct Type base;
};

struct TypeString {
    struct Type base;
};

struct TypeNil {
    struct Type base;
};

struct TypeFun {
    struct Type base;
    struct Type* params; //TODO: should be a TypeArray since that's more specific
    struct Type* ret;
};

struct TypeStruct {
    struct Type base;
    Token name;
    struct Type* super;
    struct Table props;
};

struct TypeEnum {
    struct Type base;
    Token name;
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
struct Type* make_int_type();
struct Type* make_float_type();
struct Type* make_bool_type();
struct Type* make_string_type();
struct Type* make_nil_type();
struct Type* make_fun_type(struct Type* params, struct Type* ret);
struct Type* make_struct_type(Token name, struct Type* super);
struct Type* make_identifier_type(Token identifier);
struct Type* make_list_type(struct Type* type);
struct Type* make_map_type(struct Type* type);
struct Type* make_infer_type();
struct Type* make_enum_type(Token name);

bool is_substruct(struct TypeStruct* substruct, struct TypeStruct* superstruct);
bool same_type(struct Type* type1, struct Type* type2);
bool is_primitive(struct Type* type);
void print_type(struct Type* type);

struct Type* copy_type(struct Type* type);
void free_type(struct Type* type);


#endif// CEBRA_SIG_H
