#ifndef CEBRA_OBJ_H
#define CEBRA_OBJ_H

#include <stdbool.h>

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_UPVALUE
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
    bool is_marked;
};


void insert_object(struct Obj* ptr);
void free_object(struct Obj* obj);

#endif// CEBRA_OBJ_H

