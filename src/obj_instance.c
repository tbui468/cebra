#include "obj_instance.h"
#include "memory.h"

struct ObjInstance* make_instance(struct Table* table) {
    struct ObjInstance* obj = ALLOCATE_OBJ(struct ObjInstance);
    obj->base.type = OBJ_INSTANCE;
    obj->base.next = NULL;
    insert_object((Obj*)obj);
    obj->props = table;

    return obj;
}

