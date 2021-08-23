#include <string.h>

#include "object.h"
#include "memory.h"

static void insert_object(Obj* ptr) {
    if (mm.objects == NULL) {
        mm.objects = ptr;
        return; 
    }

    ptr->next = mm.objects;
    mm.objects = ptr;
}

static uint32_t hash_string(const char* str, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* make_string(const char* start, int length) {
    char* chars = ALLOCATE_CHAR(length + 1);
    memcpy(chars, start, length);
    chars[length] = '\0';

    ObjString* obj = ALLOCATE_OBJ(ObjString);
    obj->base.type = OBJ_STRING;
    obj->base.next = NULL;
    insert_object((Obj*)obj);
    obj->chars = chars;
    obj->length = length;
    obj->hash = hash_string(chars, length);

    return obj;
}

ObjString* take_string(char* chars, int length) {
    ObjString* obj = ALLOCATE_OBJ(ObjString);
    obj->base.type = OBJ_STRING;
    obj->base.next = NULL;
    insert_object((Obj*)obj);
    obj->chars = chars;
    obj->length = length;
    obj->hash = hash_string(chars, length);

    return obj;
}


ObjFunction* make_function(Chunk chunk, int arity) {
    ObjFunction* obj = ALLOCATE_OBJ(ObjFunction);
    obj->base.type = OBJ_FUNCTION;
    obj->base.next = NULL;
    insert_object((Obj*)obj);
    obj->arity = arity;
    obj->chunk = chunk;

    return obj;
}

ObjClass* make_class(Chunk chunk) {
    ObjClass* obj = ALLOCATE_OBJ(ObjClass);
    obj->base.type = OBJ_CLASS;
    obj->base.next = NULL;
    insert_object((Obj*)obj);
    obj->chunk = chunk;

    return obj;
}


ObjInstance* make_instance(struct Table* table) {
    ObjInstance* obj = ALLOCATE_OBJ(ObjInstance);
    obj->base.type = OBJ_INSTANCE;
    obj->base.next = NULL;
    insert_object((Obj*)obj);
    obj->props = table;

    return obj;
}

void free_object(Obj* obj) {
    switch(obj->type) {
        case OBJ_STRING:
            ObjString* obj_string = (ObjString*)obj;
            FREE_CHAR(obj_string->chars, obj_string->length + 1);
            FREE_OBJ(obj_string, ObjString);
            break;
        case OBJ_FUNCTION:
            ObjFunction* obj_fun = (ObjFunction*)obj;
            free_chunk(&obj_fun->chunk);
            FREE_OBJ(obj_fun, ObjFunction);
            break;
        case OBJ_CLASS:
            ObjClass* oc = (ObjClass*)obj;
            free_chunk(&oc->chunk);
            FREE_OBJ(oc, ObjClass);
            break;
        case OBJ_INSTANCE:
            ObjInstance* oi = (ObjInstance*)obj;
//            free_table(&oi->props);
            FREE_OBJ(oi, ObjInstance);
            break;
    }
}

