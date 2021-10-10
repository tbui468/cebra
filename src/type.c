#include <stdio.h>
#include <string.h>
#include "type.h"
#include "memory.h"
#include "compiler.h"

void insert_type(struct Type* type) {
    type->next = current_compiler->types;
    type->opt = NULL;
    current_compiler->types = type;    
}

void add_type(struct TypeArray* sl, struct Type* type) {
    if (sl->count + 1 > sl->capacity) {
        int new_capacity = sl->capacity == 0 ? 8 : sl->capacity * 2;
        sl->types = GROW_ARRAY(sl->types, struct Type*, new_capacity, sl->capacity);
        sl->capacity = new_capacity;
    }

    sl->types[sl->count] = type;
    sl->count++;
}


struct Type* make_infer_type() {
    struct TypeInfer* sd = ALLOCATE(struct TypeInfer);

    sd->base.type = TYPE_INFER;

    insert_type((struct Type*)sd);
    return (struct Type*)sd;
}

struct Type* make_array_type() {
    struct TypeArray* type_list = ALLOCATE(struct TypeArray);
    type_list->types = ALLOCATE_ARRAY(struct Type*);
    type_list->count = 0;
    type_list->capacity = 0;

    type_list->base.type = TYPE_ARRAY;

    insert_type((struct Type*)type_list);
    return (struct Type*)type_list;
}

struct Type* make_int_type() {
    struct TypeInt* type = ALLOCATE(struct TypeInt);

    type->base.type = TYPE_INT;
    
    insert_type((struct Type*)type);
    return (struct Type*)type;
}
struct Type* make_float_type() {
    struct TypeFloat* type = ALLOCATE(struct TypeFloat);

    type->base.type = TYPE_FLOAT;
    
    insert_type((struct Type*)type);
    return (struct Type*)type;
}
struct Type* make_bool_type() {
    struct TypeBool* type = ALLOCATE(struct TypeBool);

    type->base.type = TYPE_BOOL;
    
    insert_type((struct Type*)type);
    return (struct Type*)type;
}
struct Type* make_string_type() {
    struct TypeString* type = ALLOCATE(struct TypeString);

    type->base.type = TYPE_STRING;
    
    insert_type((struct Type*)type);
    return (struct Type*)type;
}
struct Type* make_nil_type() {
    struct TypeNil* type = ALLOCATE(struct TypeNil);

    type->base.type = TYPE_NIL;
    
    insert_type((struct Type*)type);
    return (struct Type*)type;
}

struct Type* make_fun_type(struct Type* params, struct Type* ret) {
    struct TypeFun* type_fun = ALLOCATE(struct TypeFun);

    type_fun->base.type = TYPE_FUN;
    type_fun->params = params;
    type_fun->ret = ret;
    
    insert_type((struct Type*)type_fun);
    return (struct Type*)type_fun;
}

struct Type* make_struct_type(Token name, struct Type* super) {
    struct TypeStruct* sc = ALLOCATE(struct TypeStruct);

    sc->base.type = TYPE_STRUCT;
    sc->name = name;
    sc->super = super;
    init_table(&sc->props);

    insert_type((struct Type*)sc);
    return (struct Type*)sc;
}

struct Type* make_identifier_type(Token identifier) {
    struct TypeIdentifier* si = ALLOCATE(struct TypeIdentifier);

    si->base.type = TYPE_IDENTIFIER;
    si->identifier = identifier;

    insert_type((struct Type*)si);
    return (struct Type*)si;
}

struct Type* make_list_type(struct Type* type) {
    struct TypeList* sl = ALLOCATE(struct TypeList);

    sl->base.type = TYPE_LIST;
    sl->type = type;

    insert_type((struct Type*)sl);
    return (struct Type*)sl;
}

struct Type* make_map_type(struct Type* type) {
    struct TypeMap* sm = ALLOCATE(struct TypeMap);

    sm->base.type = TYPE_MAP;
    sm->type = type;

    insert_type((struct Type*)sm);
    return (struct Type*)sm;
}

struct Type* make_enum_type(Token name) {
    struct TypeEnum* te = ALLOCATE(struct TypeEnum);

    te->base.type = TYPE_ENUM;
    te->name = name;
    init_table(&te->props);

    insert_type((struct Type*)te);
    return (struct Type*)te;
}


bool is_primitive(struct Type* type) {
    return type->type == TYPE_INT ||
           type->type == TYPE_FLOAT ||
           type->type == TYPE_BOOL ||
           type->type == TYPE_STRING ||
           type->type == TYPE_NIL;
}

bool is_substruct(struct TypeStruct* substruct, struct TypeStruct* superstruct) {
    if (substruct->super == NULL) return false;

    struct Type* t = substruct->super;
    while (t != NULL) {
        Token super_tok = ((struct TypeStruct*)t)->name;
        Token class_tok = superstruct->name;
        if (super_tok.length == class_tok.length && memcmp(super_tok.start, class_tok.start, super_tok.length) == 0) {
            return true;
        }
        t = ((struct TypeStruct*)t)->super;
    }

    return false;
}

bool same_type(struct Type* type1, struct Type* type2) {
    if (type1 == NULL || type2 == NULL) return false;
    if (type1->type == TYPE_NIL || type2->type == TYPE_NIL) return true;

    switch(type1->type) {
        case TYPE_ARRAY: {
            struct TypeArray* sl1 = (struct TypeArray*)type1;
            struct TypeArray* sl2 = (struct TypeArray*)type2;
            if (sl1->count != sl2->count) return false;
            for (int i = 0; i < sl1->count; i++) {
                if (!same_type(sl1->types[i], sl2->types[i])) return false;
            }
            return true;
        }
        case TYPE_FUN: {
            struct TypeFun* sf1 = (struct TypeFun*)type1;
            struct TypeFun* sf2 = (struct TypeFun*)type2;
            return same_type(sf1->ret, sf2->ret) && same_type(sf1->params, sf2->params);
        }
        case TYPE_STRUCT: {
            struct TypeStruct* sc1 = (struct TypeStruct*)type1;
            struct TypeStruct* sc2 = (struct TypeStruct*)type2;
            return sc1->name.length == sc2->name.length && memcmp(sc1->name.start, sc2->name.start, sc1->name.length) == 0;
        }
        case TYPE_IDENTIFIER: {
            struct TypeIdentifier* si1 = (struct TypeIdentifier*)type1; 
            struct TypeIdentifier* si2 = (struct TypeIdentifier*)type2; 
            if (si1->identifier.length != si2->identifier.length) return false;
            return memcmp(si1->identifier.start, si2->identifier.start, si1->identifier.length) == 0;
        }
        case TYPE_LIST: {
            return same_type(((struct TypeList*)type1)->type, ((struct TypeList*)type2)->type);
        }
        case TYPE_MAP: {
            return same_type(((struct TypeMap*)type1)->type, ((struct TypeMap*)type2)->type);
        }
        case TYPE_ENUM: {
            struct TypeEnum* te1 = (struct TypeEnum*)type1;
            struct TypeEnum* te2 = (struct TypeEnum*)type2;
            if (te1->name.length != te2->name.length) return false;
            if (memcmp(te1->name.start, te2->name.start, te1->name.length) != 0) return false;
            return true;
        }
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_BOOL:
        case TYPE_STRING:
        case TYPE_INFER:
        default: {
            struct Type* left = type1;
            while (left != NULL) {
                struct Type* right = type2;
                while (right != NULL) {
                    if (left->type == right->type) return true;
                    right = right->opt;
                }
                left = left->opt;
            }
            return false;
        }
    }
}

void print_type(struct Type* type) {
    switch(type->type) {
        case TYPE_INFER: {
            printf("TypeInfer");
            break;
        }
        case TYPE_ARRAY: {
            struct TypeArray* sl = (struct TypeArray*)type;
            printf("(");
            for (int i = 0; i < sl->count; i++) {
                print_type(sl->types[i]);
            }
            printf(")");
            break;
        }
        case TYPE_FUN: {
            struct TypeFun* sf = (struct TypeFun*)type;
            struct TypeArray* params = (struct TypeArray*)(sf->params);
            printf("(");
            for (int i = 0; i < params->count; i++) {
                print_type(params->types[i]);
            } 
            printf(") -> ");
            print_type(sf->ret);
            break;
        }
        case TYPE_STRUCT: {
            struct TypeStruct* sc = (struct TypeStruct*)type;
            printf("( TypeStruct Stub");
            print_token(sc->name);
            printf(" )");
            break;
        }
        case TYPE_ENUM: {
            printf("TypeEnum Stub");
            break;
        }
        case TYPE_IDENTIFIER: {
            struct TypeIdentifier* ti = (struct TypeIdentifier*)type;
            printf("TypeIdentifier [%.*s]", ti->identifier.length, ti->identifier.start);
            break;
        }
        case TYPE_LIST: {
            printf("TypeList Stub");
            break;
        }
        case TYPE_MAP: {
            printf("TypeMap Stub");
            break;
        }
        case TYPE_INT: {
            printf("TypeInt stub");
            break;
        }
        case TYPE_FLOAT: {
            printf("TypeFloat stub");
            break;
        }
        case TYPE_BOOL: {
            printf("TypeBool stub");
            break;
        }
        case TYPE_STRING: {
            printf("TypeString stub");
            break;
        }
        case TYPE_NIL: {
            printf("TypeNil stub");
            break;
        }
        default: {
            printf("Invalid type.");
            break;
        }
    }
}

void free_type(struct Type* type) {
    switch(type->type) {
        case TYPE_INFER: {
            struct TypeInfer* sd = (struct TypeInfer*)type;
            FREE(sd, struct TypeInfer);
            break;
        }
        case TYPE_ARRAY: {
            struct TypeArray* sa = (struct TypeArray*)type;
            FREE_ARRAY(sa->types, struct Type*, sa->capacity);
            FREE(sa, struct TypeArray);
            break;
        }
        case TYPE_FUN: {
            struct TypeFun* type_fun = (struct TypeFun*)type;
            FREE(type_fun, struct TypeFun);
            break;
        }
        case TYPE_STRUCT: {
            struct TypeStruct* sc = (struct TypeStruct*)type;
            free_table(&sc->props);
            FREE(sc, struct TypeStruct);
            break;
        }
        case TYPE_ENUM: {
            struct TypeEnum* te = (struct TypeEnum*)type;
            free_table(&te->props);
            FREE(te, struct TypeEnum);
            break;
        }
        case TYPE_IDENTIFIER: {
            struct TypeIdentifier* si = (struct TypeIdentifier*)type;
            FREE(si, struct TypeIdentifier);
            break;
        }
        case TYPE_LIST: {
            struct TypeList* sl = (struct TypeList*)type;
            FREE(sl, struct TypeList);
            break;
        }
        case TYPE_MAP: {
            struct TypeMap* sm = (struct TypeMap*)type;
            FREE(sm, struct TypeMap);
            break;
        }
        case TYPE_INT: {
            struct TypeInt* type_int = (struct TypeInt*)type;
            FREE(type_int, struct TypeInt);
            break;
        }
        case TYPE_FLOAT: {
            struct TypeFloat* type_float = (struct TypeFloat*)type;
            FREE(type_float, struct TypeFloat);
            break;
        }
        case TYPE_BOOL: {
            struct TypeBool* type_bool = (struct TypeBool*)type;
            FREE(type_bool, struct TypeBool);
            break;
        }
        case TYPE_STRING: {
            struct TypeString* type_string = (struct TypeString*)type;
            FREE(type_string, struct TypeString);
            break;
        }
        case TYPE_NIL: {
            struct TypeNil* type_nil = (struct TypeNil*)type;
            FREE(type_nil, struct TypeNil);
            break;
        }
    }
}
