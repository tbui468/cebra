#include "obj_class.h"
#include "memory.h"


struct ObjClass* make_class(struct ObjString* name) {
    struct ObjClass* obj = ALLOCATE(struct ObjClass);
    push_root(to_class(obj));
    obj->base.type = OBJ_CLASS;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->name = name;
    init_table(&obj->props);

    pop_root();
    return obj;
}

struct ObjInstance* make_instance(struct Table table) {
    struct ObjInstance* obj = ALLOCATE(struct ObjInstance);
    obj->base.type = OBJ_INSTANCE;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);
    obj->props = table;

    return obj;
}

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

struct ObjList* make_list() {
    struct ObjList* obj = ALLOCATE(struct ObjList);
    push_root(to_list(obj));
    obj->base.type = OBJ_LIST;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    obj->default_value = to_nil();
    insert_object((struct Obj*)obj);

    init_value_array(&obj->values);

    pop_root();
    return obj;
}

struct ObjMap* make_map() {
    struct ObjMap* obj = ALLOCATE(struct ObjMap);
    push_root(to_map(obj)); 
    obj->base.type = OBJ_MAP;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    obj->default_value = to_nil();
    insert_object((struct Obj*)obj);

    init_table(&obj->table);

    pop_root();
    return obj;
}
