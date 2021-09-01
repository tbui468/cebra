#include "obj.h"
#include "obj_instance.h"
#include "obj_function.h"
#include "obj_class.h"
#include "obj_string.h"
#include "memory.h"

void insert_object(struct Obj* ptr) {
    if (mm.objects == NULL) {
        mm.objects = ptr;
        return; 
    }

    ptr->next = mm.objects;
    mm.objects = ptr;
}


int free_object(struct Obj* obj) {
    int bytes_freed = 0;
    switch(obj->type) {
        case OBJ_STRING: {
            struct ObjString* obj_string = (struct ObjString*)obj;
            bytes_freed += FREE_ARRAY(obj_string->chars, char, obj_string->length + 1);
            bytes_freed += FREE(obj_string, struct ObjString);
            break;
        }
        case OBJ_FUNCTION:
            struct ObjFunction* obj_fun = (struct ObjFunction*)obj;
            bytes_freed += free_chunk(&obj_fun->chunk);
            bytes_freed += FREE(obj_fun, struct ObjFunction);
            break;
        case OBJ_CLASS:
            struct ObjClass* oc = (struct ObjClass*)obj;
            bytes_freed += free_chunk(&oc->chunk);
            bytes_freed += FREE(oc, struct ObjClass);
            break;
        case OBJ_INSTANCE:
            struct ObjInstance* oi = (struct ObjInstance*)obj;
//            free_table(&oi->props);
            bytes_freed += FREE(oi, struct ObjInstance);
            break;
        case OBJ_UPVALUE:
            struct ObjUpvalue* uv = (struct ObjUpvalue*)obj;
            //free the value if closed
            if (IS_STRING(uv->closed)) {
                bytes_freed += free_object((struct Obj*)(uv->closed.as.string_type));
            }
            if (IS_FUNCTION(uv->closed)) {
                bytes_freed += free_object((struct Obj*)uv->closed.as.function_type);
            }
            if (IS_CLASS(uv->closed)) {
                bytes_freed += free_object((struct Obj*)uv->closed.as.class_type);
            }
            if (IS_INSTANCE(uv->closed)) {
                bytes_freed += free_object((struct Obj*)uv->closed.as.instance_type);
            }
            bytes_freed += FREE(uv, struct ObjUpvalue);
            break;
    }
    return bytes_freed;
}


void mark_object(struct Obj* obj) {
    obj->is_marked = true;
}

