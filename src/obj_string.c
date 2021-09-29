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

struct ObjString* take_string(char* chars, int length) {
    struct ObjString* obj = ALLOCATE(struct ObjString);

    obj->base.type = OBJ_STRING;
    obj->base.next = NULL;
    obj->base.is_marked = false;

    obj->chars = chars;
    obj->length = length;
    obj->hash = hash_string(chars, length);

    insert_object((struct Obj*)obj);
    return obj;
}

struct ObjString* make_string(const char* start, int length) {
    char* chars = ALLOCATE_ARRAY(char);
    chars = GROW_ARRAY(chars, char, length + 1, 0);
    memcpy(chars, start, length);
    chars[length] = '\0';

    return take_string(chars, length);
}

void swap_strings(struct ObjString* str1, struct ObjString* str2) {
    struct ObjString temp = *str1;
    *str1 = *str2;
    *str2 = temp;

    //resulting str1 ('abcde') is not in linked list and not deleted
}
