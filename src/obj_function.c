
#include "memory.h"
#include "obj_function.h"


struct ObjFunction* make_function(struct ObjString* name, int arity) {
    struct ObjFunction* obj = ALLOCATE(struct ObjFunction);
    push_root(to_function(obj));
    obj->base.type = OBJ_FUNCTION;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->name = name;
    obj->arity = arity;
    init_chunk(&obj->chunk);
    obj->upvalue_count = 0;

    pop_root();
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

struct ObjNative* make_native(Value (*function)(int, Value*)) {
    struct ObjNative* obj = ALLOCATE(struct ObjNative);
    push_root(to_native(obj));
    obj->base.type = OBJ_NATIVE;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->function = function;

    pop_root();
    return obj;
}

struct ObjList* make_list(Value default_value) {
    struct ObjList* obj = ALLOCATE(struct ObjList);
    push_root(to_list(obj));
    obj->base.type = OBJ_LIST;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    init_value_array(&obj->values);
    obj->default_value = default_value;
    obj->count = 0;
    obj->capacity = 0;

    pop_root();
    return obj;
}




