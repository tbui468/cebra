#ifndef CEBRA_OBJ_INSTANCE_H
#define CEBRA_OBJ_INSTANCE_H

#include "obj.h"
#include "table.h"

struct ObjInstance {
    Obj base;
    struct Table* props; //both for both data and methods
};

struct ObjInstance* make_instance(struct Table* table);

#endif// CEBRA_OBJ_INSTANCE_H
