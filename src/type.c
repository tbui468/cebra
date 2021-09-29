#include <stdio.h>
#include <string.h>
#include "type.h"
#include "memory.h"
#include "compiler.h"

void insert_type(struct Type* type) {
    type->next = current_compiler->types;
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


struct Type* make_decl_type() {
    struct TypeDecl* sd = ALLOCATE(struct TypeDecl);

    sd->base.type = TYPE_DECL;
    sd->base.opt = NULL;

    insert_type((struct Type*)sd);
    return (struct Type*)sd;
}

struct Type* make_array_type() {
    struct TypeArray* type_list = ALLOCATE(struct TypeArray);
    type_list->types = ALLOCATE_ARRAY(struct Type*);
    type_list->count = 0;
    type_list->capacity = 0;

    type_list->base.type = TYPE_ARRAY;
    type_list->base.opt = NULL;

    insert_type((struct Type*)type_list);
    return (struct Type*)type_list;
}

struct Type* make_prim_type(ValueType type) {
    struct TypePrim* type_prim = ALLOCATE(struct TypePrim);

    type_prim->base.type = TYPE_PRIM;
    type_prim->base.opt = NULL;
    type_prim->type = type;
    
    insert_type((struct Type*)type_prim);
    return (struct Type*)type_prim;
}

struct Type* make_fun_type(struct Type* params, struct Type* ret) {
    struct TypeFun* type_fun = ALLOCATE(struct TypeFun);

    type_fun->base.type = TYPE_FUN;
    type_fun->base.opt = NULL;
    type_fun->params = params;
    type_fun->ret = ret;
    
    insert_type((struct Type*)type_fun);
    return (struct Type*)type_fun;
}

struct Type* make_class_type(Token klass) {
    struct TypeClass* sc = ALLOCATE(struct TypeClass);

    sc->base.type = TYPE_CLASS;
    sc->base.opt = NULL;
    sc->klass = klass;
    init_table(&sc->props);

    insert_type((struct Type*)sc);
    return (struct Type*)sc;
}

struct Type* make_identifier_type(Token identifier) {
    struct TypeIdentifier* si = ALLOCATE(struct TypeIdentifier);

    si->base.type = TYPE_IDENTIFIER;
    si->base.opt = NULL;
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

bool is_duck(struct TypeClass* param, struct TypeClass* arg) {
    for (int i = 0; i < param->props.capacity; i++) {
        struct Pair* param_pair = &param->props.pairs[i];
        if (param_pair->key != NULL) {
            Value arg_value;
            if (!get_from_table(&arg->props, param_pair->key, &arg_value))
                return false;

            struct Type* param_type = param_pair->value.as.type_type;
            struct Type* arg_type = arg_value.as.type_type;

            if (IS_CLASS(param_pair->value) && IS_CLASS(arg_value)) {
                if (!is_duck((struct TypeClass*)param_type, (struct TypeClass*)arg_type)) {
                    return false;
                }
            } else {
                if (!same_type(param_type, arg_type)) return false;
            }
        }
    }
    return true;
}

bool same_type(struct Type* type1, struct Type* type2) {
    if (type1 == NULL || type2 == NULL) return false;
    if (type_is_type(type1, VAL_NIL) || type_is_type(type2, VAL_NIL)) return true;
    if (type1->type != type2->type) return false;

    switch(type1->type) {
        case TYPE_ARRAY:
            struct TypeArray* sl1 = (struct TypeArray*)type1;
            struct TypeArray* sl2 = (struct TypeArray*)type2;
            if (sl1->count != sl2->count) return false;
            for (int i = 0; i < sl1->count; i++) {
                if (!same_type(sl1->types[i], sl2->types[i])) return false;
            }
            return true;
        case TYPE_PRIM: {
            struct Type* left = type1;
            while (left != NULL) {
                struct Type* right = type2;
                while (right != NULL) {
                    if (((struct TypePrim*)left)->type == ((struct TypePrim*)right)->type) return true;
                    right = right->opt;
                }
                left = left->opt;
            }
            return false;
        }
        case TYPE_FUN:
            struct TypeFun* sf1 = (struct TypeFun*)type1;
            struct TypeFun* sf2 = (struct TypeFun*)type2;
            return same_type(sf1->ret, sf2->ret) && same_type(sf1->params, sf2->params);
        case TYPE_CLASS:
            struct TypeClass* sc1 = (struct TypeClass*)type1;
            struct TypeClass* sc2 = (struct TypeClass*)type2;
            //only checking typenature types (and not recursively comparing properties that are struct types)
            //since structs can hold references to themselves.  Doing so could loop indefinitely.
            for (int i = 0; i < sc1->props.capacity; i++) {
                struct Pair* pair = &sc1->props.pairs[i];
                if (pair->key != NULL) {
                    Value value;
                    if (!get_from_table(&sc2->props, pair->key, &value))
                        return false;
                    if (pair->value.as.type_type->type != value.as.type_type->type)
                        return false;
                }
            }
            return true;
        case TYPE_IDENTIFIER:
            struct TypeIdentifier* si1 = (struct TypeIdentifier*)type1; 
            struct TypeIdentifier* si2 = (struct TypeIdentifier*)type2; 
            if (si1->identifier.length != si2->identifier.length) return false;
            return memcmp(si1->identifier.start, si2->identifier.start, si1->identifier.length) == 0;
        case TYPE_LIST:
            return same_type(((struct TypeList*)type1)->type, ((struct TypeList*)type2)->type);
        case TYPE_MAP:
            return same_type(((struct TypeMap*)type1)->type, ((struct TypeMap*)type2)->type);
        case TYPE_DECL:
            return true;
    }
}

bool type_is_type(struct Type* type, ValueType val) {
    if (type->type != TYPE_PRIM) return false;

    struct Type* current = type;
    while (current != NULL) {
        if (((struct TypePrim*)type)->type == val) return true; 
        current = current->opt;
    }

    return false;
}

void print_type(struct Type* type) {
    switch(type->type) {
        case TYPE_DECL:
            printf("TypeDecl");
            break;
        case TYPE_ARRAY:
            struct TypeArray* sl = (struct TypeArray*)type;
            printf("(");
            for (int i = 0; i < sl->count; i++) {
                print_type(sl->types[i]);
            }
            printf(")");
            break;
        case TYPE_PRIM:
            struct TypePrim* sp = (struct TypePrim*)type;
            printf(" %s ", value_type_to_string(sp->type));
            break;
        case TYPE_FUN:
            struct TypeFun* sf = (struct TypeFun*)type;
            struct TypeArray* params = (struct TypeArray*)(sf->params);
            printf("(");
            for (int i = 0; i < params->count; i++) {
                print_type(params->types[i]);
            } 
            printf(") -> ");
            print_type(sf->ret);
            break;
        case TYPE_CLASS:
            struct TypeClass* sc = (struct TypeClass*)type;
            printf("( TypeClass Stub");
            print_token(sc->klass);
            printf(" )");
            break;
        case TYPE_IDENTIFIER:
            printf("TypeIdentifier Stub");
            break;
        case TYPE_LIST:
            printf("TypeList Stub");
            break;
        case TYPE_MAP:
            printf("TypeMap Stub");
            break;
        default:
            printf("Invalid type");
            break;
    }
}

void free_type(struct Type* type) {
    switch(type->type) {
        case TYPE_DECL:
            struct TypeDecl* sd = (struct TypeDecl*)type;
            FREE(sd, struct TypeDecl);
            break;
        case TYPE_ARRAY:
            struct TypeArray* sa = (struct TypeArray*)type;
            FREE_ARRAY(sa->types, struct Type*, sa->capacity);
            FREE(sa, struct TypeArray);
            break;
        case TYPE_PRIM:
            struct TypePrim* type_prim = (struct TypePrim*)type;
            FREE(type_prim, struct TypePrim);
            break;
        case TYPE_FUN:
            struct TypeFun* type_fun = (struct TypeFun*)type;
            FREE(type_fun, struct TypeFun);
            break;
        case TYPE_CLASS:
            struct TypeClass* sc = (struct TypeClass*)type;
            free_table(&sc->props);
            FREE(sc, struct TypeClass);
            break;
        case TYPE_IDENTIFIER:
            struct TypeIdentifier* si = (struct TypeIdentifier*)type;
            FREE(si, struct TypeIdentifier);
            break;
        case TYPE_LIST:
            struct TypeList* sl = (struct TypeList*)type;
            FREE(sl, struct TypeList);
            break;
        case TYPE_MAP:
            struct TypeMap* sm = (struct TypeMap*)type;
            FREE(sm, struct TypeMap);
            break;
    }
}
