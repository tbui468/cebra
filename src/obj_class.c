#include "obj_class.h"
#include "memory.h"


struct ObjClass* make_class(Chunk chunk) {
    struct ObjClass* obj = ALLOCATE_OBJ(struct ObjClass);
    obj->base.type = OBJ_CLASS;
    obj->base.next = NULL;
    insert_object((struct Obj*)obj);
    obj->chunk = chunk;

    return obj;
}
