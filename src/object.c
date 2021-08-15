#include <string.h>

#include "object.h"
#include "memory.h"


ObjString* make_string(const char* start, int length) {
    char* chars = ALLOCATE_CHAR(length + 1);
    memcpy(chars, start, length);
    chars[length] = '\0';

    ObjString* obj = ALLOCATE_OBJ(ObjString);
    obj->base.type = OBJ_STRING;
    obj->chars = chars;
    obj->length = length;

    return obj;
}

ObjString* take_string(char* chars, int length) {
    ObjString* obj = ALLOCATE_OBJ(ObjString);
    obj->base.type = OBJ_STRING;
    obj->chars = chars;
    obj->length = length;

    return obj;
}


ObjFunction* make_function(Chunk chunk, int arity) {
    ObjFunction* obj = ALLOCATE_OBJ(ObjFunction);
    obj->base.type = OBJ_FUNCTION;
    obj->arity = arity;
    obj->chunk = chunk;

    return obj;
}
