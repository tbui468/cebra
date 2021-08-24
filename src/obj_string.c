#include <string.h>

#include "obj_string.h"
#include "memory.h"


static uint32_t hash_string(const char* str, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619;
    }
    return hash;
}

struct ObjString* make_string(const char* start, int length) {
    char* chars = ALLOCATE_CHAR(length + 1);
    memcpy(chars, start, length);
    chars[length] = '\0';

    struct ObjString* obj = ALLOCATE_OBJ(struct ObjString);
    obj->base.type = OBJ_STRING;
    obj->base.next = NULL;
    insert_object((struct Obj*)obj);
    obj->chars = chars;
    obj->length = length;
    obj->hash = hash_string(chars, length);

    return obj;
}

struct ObjString* take_string(char* chars, int length) {
    struct ObjString* obj = ALLOCATE_OBJ(struct ObjString);
    obj->base.type = OBJ_STRING;
    obj->base.next = NULL;
    insert_object((struct Obj*)obj);
    obj->chars = chars;
    obj->length = length;
    obj->hash = hash_string(chars, length);

    return obj;
}
