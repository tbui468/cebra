#ifndef CEBRA_OBJ_FUNCTION_H
#define CEBRA_OBJ_FUNCTION_H

#include "chunk.h"
#include "obj.h"
#include "table.h"

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

struct ObjFunction* make_function(struct ObjString* name, int arity);
struct ObjUpvalue* make_upvalue(Value* location);
struct ObjNative* make_native(Value (*function)(int, Value*));

#endif// CEBRA_OBJ_FUNCTION_H
