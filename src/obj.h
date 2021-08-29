#ifndef CEBRA_OBJ_H
#define CEBRA_OBJ_H

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
};


void insert_object(struct Obj* ptr);
void free_object(struct Obj* obj);

#endif// CEBRA_OBJ_H

