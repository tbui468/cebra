#ifndef CEBRA_OBJ_H
#define CEBRA_OBJ_H

#include <stdbool.h>
#include "chunk.h"
#include "obj.h"
#include "table.h"


typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_UPVALUE,
    OBJ_NATIVE,
    OBJ_LIST,
    OBJ_MAP
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
    bool is_marked;
};

struct ObjClass {
    struct Obj base;
    struct ObjString* name;
    struct Table props;
};

struct ObjInstance {
    struct Obj base;
    struct Table props;
};

struct ObjUpvalue {
    struct Obj base;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
};

struct ObjFunction {
    struct Obj base;
    struct ObjString* name;
    int arity;
    Chunk chunk;
    struct ObjUpvalue* upvalues[256];
    int upvalue_count;
};

struct ObjNative {
    struct Obj base;
    Value (*function)(int, Value*);
};

struct ObjList {
    struct Obj base;
    struct ValueArray values;
    Value default_value;
};

struct ObjMap {
    struct Obj base;
    struct Table table;
    Value default_value;
};

struct ObjInstance* make_instance(struct Table table);
struct ObjClass* make_class(struct ObjString* name);
struct ObjFunction* make_function(struct ObjString* name, int arity);
struct ObjUpvalue* make_upvalue(Value* location);
struct ObjNative* make_native(Value (*function)(int, Value*));
struct ObjList* make_list();
struct ObjMap* make_map();


void insert_object(struct Obj* ptr);
int free_object(struct Obj* obj);
void mark_object(struct Obj* obj);
void print_object(struct Obj* obj);

#endif// CEBRA_OBJ_H

