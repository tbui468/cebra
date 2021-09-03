#ifndef CEBRA_OBJ_CLASS_H
#define CEBRA_OBJ_CLASS_H

#include "chunk.h"
#include "obj.h"
#include "table.h"

struct ObjClass {
    struct Obj base;
    struct ObjString* name;
    struct Table properties;
};

struct ObjClass* make_class(struct ObjString* name);


#endif// CEBRA_OBJ_CLASS_H
