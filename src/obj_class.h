#ifndef CEBRA_OBJ_CLASS_H
#define CEBRA_OBJ_CLASS_H

#include "chunk.h"
#include "obj.h"
#include "table.h"

struct ObjClass {
    struct Obj base;
    struct ObjString* name;
    struct Table props;
};


struct ObjInstance {
    struct Obj base;
    struct Table props;
};

struct ObjInstance* make_instance(struct Table table);
struct ObjClass* make_class(struct ObjString* name);


#endif// CEBRA_OBJ_CLASS_H
