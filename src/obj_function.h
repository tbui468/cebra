#ifndef CEBRA_OBJ_FUNCTION_H
#define CEBRA_OBJ_FUNCTION_H

#include "chunk.h"
#include "obj.h"
#include "table.h"

struct ObjUpvalue {
    struct Obj base;
    Value* value;
};

struct ObjFunction {
    struct Obj base;
    int arity;
    Chunk chunk;
    struct ObjUpvalue* upvalues[256];
    int upvalue_count;
};

struct ObjFunction* make_function(Chunk chunk, int arity);
struct ObjUpvalue* make_upvalue(Value* value);

#endif// CEBRA_OBJ_FUNCTION_H
