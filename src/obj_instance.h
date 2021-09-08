#ifndef CEBRA_OBJ_INSTANCE_H
#define CEBRA_OBJ_INSTANCE_H

#include "obj.h"
#include "table.h"

struct ObjInstance {
    struct Obj base;
    struct Table props;
};

struct ObjInstance* make_instance(struct Table table);

#endif// CEBRA_OBJ_INSTANCE_H
