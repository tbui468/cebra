#ifndef CEBRA_OBJ_FUNCTION_H
#define CEBRA_OBJ_FUNCTION_H

#include "chunk.h"
#include "obj.h"
#include "table.h"

struct Upvalue {
    int index;
};

struct ObjFunction {
    struct Obj base;
    int arity;
    Chunk chunk;
    struct Upvalue upvalues[256];
    int upvalue_count;
};

struct ObjFunction* make_function(Chunk chunk, int arity);

#endif// CEBRA_OBJ_FUNCTION_H
