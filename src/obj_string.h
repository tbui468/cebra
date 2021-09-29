#ifndef CEBRA_OBJ_STRING_H
#define CEBRA_OBJ_STRING_H

#include <stdint.h>

#include "obj.h"

struct ObjString {
    struct Obj base;
    char* chars;
    int length;
    uint32_t hash;
};

struct ObjString* make_string(const char* start, int length);
struct ObjString* take_string(char* start, int length);

#endif// CEBRA_OBJ_STRING_H
