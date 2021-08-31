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
    char* chars = ALLOCATE_ARRAY(char);
    chars = GROW_ARRAY(chars, char, length + 1, 0);
    memcpy(chars, start, length);
    chars[length] = '\0';

    struct ObjString* obj = ALLOCATE(struct ObjString);
    obj->base.type = OBJ_STRING;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);
    obj->chars = chars;
    obj->length = length;
    obj->hash = hash_string(chars, length);

    return obj;
}

struct ObjString* take_string(char* chars, int length) {
    struct ObjString* obj = ALLOCATE(struct ObjString);
    obj->base.type = OBJ_STRING;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);
    obj->chars = chars;
    obj->length = length;
    obj->hash = hash_string(chars, length);

    return obj;
}
