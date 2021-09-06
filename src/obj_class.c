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
