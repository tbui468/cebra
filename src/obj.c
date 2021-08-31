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


void free_object(struct Obj* obj) {
    switch(obj->type) {
        case OBJ_STRING:
            struct ObjString* obj_string = (struct ObjString*)obj;
            FREE_ARRAY(obj_string->chars, char, obj_string->length + 1);
            FREE(obj_string, struct ObjString);
            break;
        case OBJ_FUNCTION:
            struct ObjFunction* obj_fun = (struct ObjFunction*)obj;
            free_chunk(&obj_fun->chunk);
            FREE(obj_fun, struct ObjFunction);
            break;
        case OBJ_CLASS:
            struct ObjClass* oc = (struct ObjClass*)obj;
            free_chunk(&oc->chunk);
            FREE(oc, struct ObjClass);
            break;
        case OBJ_INSTANCE:
            struct ObjInstance* oi = (struct ObjInstance*)obj;
//            free_table(&oi->props);
            FREE(oi, struct ObjInstance);
            break;
        case OBJ_UPVALUE:
            struct ObjUpvalue* uv = (struct ObjUpvalue*)obj;
            //free the value if closed
            if (IS_STRING(uv->closed)) {
                free_object((struct Obj*)(uv->closed.as.string_type));
            }
            if (IS_FUNCTION(uv->closed)) {
                free_object((struct Obj*)uv->closed.as.function_type);
            }
            if (IS_CLASS(uv->closed)) {
                free_object((struct Obj*)uv->closed.as.class_type);
            }
            if (IS_INSTANCE(uv->closed)) {
                free_object((struct Obj*)uv->closed.as.instance_type);
            }
            FREE(uv, struct ObjUpvalue);
            break;
    }
}

