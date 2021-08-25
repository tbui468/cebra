#ifndef CEBRA_OBJ_FUNCTION_H
#define CEBRA_OBJ_FUNCTION_H

#include "chunk.h"
#include "obj.h"
#include "table.h"

struct ObjFunction {
    struct Obj base;
    int arity;
    Chunk chunk;
};

struct ObjFunction* make_function(Chunk chunk, int arity);

#endif// CEBRA_OBJ_FUNCTION_H
