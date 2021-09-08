#include "obj_instance.h"
#include "memory.h"

struct ObjInstance* make_instance(struct Table table) {
    struct ObjInstance* obj = ALLOCATE(struct ObjInstance);
    obj->base.type = OBJ_INSTANCE;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);
    obj->props = table;

    return obj;
}

