
#include "memory.h"
#include "obj_function.h"


struct ObjFunction* make_function(Chunk chunk, int arity) {
    struct ObjFunction* obj = ALLOCATE_OBJ(struct ObjFunction);
    obj->base.type = OBJ_FUNCTION;
    obj->base.next = NULL;
    insert_object((struct Obj*)obj);
    obj->arity = arity;
    obj->chunk = chunk;
    obj->upvalue_count = 0;

    return obj;
}
