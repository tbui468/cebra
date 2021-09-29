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
void swap_strings(struct ObjString* str1, struct ObjString* str2);

#endif// CEBRA_OBJ_STRING_H
