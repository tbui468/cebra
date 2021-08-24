#ifndef CEBRA_OBJ_H
#define CEBRA_OBJ_H

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLASS,
    OBJ_INSTANCE
} ObjType;

typedef struct {
    ObjType type;
    struct Obj* next;
} Obj;


void insert_object(Obj* ptr);
void free_object(Obj* obj);

#endif// CEBRA_OBJ_H

