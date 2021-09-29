#include "obj.h"
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
            //NOTE: this only frees the table entries - the key and any heap allocated values will
            //be freed by the GC
            bytes_freed += free_table(&oc->props);
            bytes_freed += FREE(oc, struct ObjClass);
            break;
        case OBJ_INSTANCE:
            struct ObjInstance* oi = (struct ObjInstance*)obj;
            bytes_freed += free_table(&oi->props);
            bytes_freed += FREE(oi, struct ObjInstance);
            break;
        case OBJ_UPVALUE:
            struct ObjUpvalue* uv = (struct ObjUpvalue*)obj;
            bytes_freed += FREE(uv, struct ObjUpvalue);
            break;
        case OBJ_NATIVE:
            struct ObjNative* nat = (struct ObjNative*)obj;
            bytes_freed += FREE(nat, struct ObjNative);
            break;
        case OBJ_LIST:
            struct ObjList* list = (struct ObjList*)obj;
            bytes_freed += free_value_array(&list->values);
            bytes_freed += FREE(list, struct ObjList);
            break;
        case OBJ_MAP:
            struct ObjMap* map = (struct ObjMap*)obj;
            bytes_freed += free_table(&map->table);
            bytes_freed += FREE(map, struct ObjMap);
            break;
    }
    return bytes_freed;
}


void print_object(struct Obj* obj) {
    switch(obj->type) {
        case OBJ_STRING:
            printf("OBJ_STRING [");
            print_value(to_string((struct ObjString*)obj));
            printf("] : ");
            break;
        case OBJ_FUNCTION:
            printf("OBJ_FUNCTION [");
            print_value(to_function((struct ObjFunction*)obj));
            printf("] : ");
            break;
        case OBJ_CLASS:
            printf("OBJ_CLASS: ");
            break;
        case OBJ_INSTANCE:
            printf("OBJ_INSTANCE: ");
            break;
        case OBJ_UPVALUE:
            printf("OBJ_UPVALUE: ");
            print_value(((struct ObjUpvalue*)obj)->closed);
            printf(" ");
            break;
        case OBJ_NATIVE:
            printf("OBJ_NATIVE [");
            print_value(to_native((struct ObjNative*)obj));
            printf("] : ");
            break;
        case OBJ_LIST:
            printf("OBJ_LIST:");
            break;
        case OBJ_MAP:
            printf("OBJ_MAP:");
            break;
        default:
            printf("Invalid Object: ");
            break;
    }
    if (obj->is_marked) {
        printf("is marked, ");
    } else {
        printf("NOT marked, ");
    }
}

void mark_object(struct Obj* obj) {
    if (obj == NULL) return;
    obj->is_marked = true;
}

