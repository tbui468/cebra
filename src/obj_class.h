#ifndef CEBRA_OBJ_CLASS_H
#define CEBRA_OBJ_CLASS_H

#include "chunk.h"
#include "obj.h"

struct ObjClass{
    Obj base;
    Chunk chunk;
};

struct ObjClass* make_class(Chunk chunk);


#endif// CEBRA_OBJ_CLASS_H
