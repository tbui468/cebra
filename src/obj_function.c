
#include "memory.h"
#include "obj_function.h"


struct ObjFunction* make_function(Chunk chunk, int arity) {
    struct ObjFunction* obj = ALLOCATE(struct ObjFunction);
    obj->base.type = OBJ_FUNCTION;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);
    obj->arity = arity;
    obj->chunk = chunk;
    obj->upvalue_count = 0;

    return obj;
}

struct ObjUpvalue* make_upvalue(Value* location) {
    struct ObjUpvalue* obj = ALLOCATE(struct ObjUpvalue);
    obj->base.type = OBJ_UPVALUE;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->location = location;
    obj->closed = to_nil();
    obj->next = NULL;

    return obj;
}
