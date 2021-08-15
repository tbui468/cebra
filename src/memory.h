#ifndef CEBRA_MEMORY_H
#define CEBRA_MEMORY_H

#include <stdlib.h>

//AST Nodes - GC shouldn't need to worry about these.
//node lists should free them after compiling, before the VM
//even needs to see them.
//Arrays (the c ones) shouldn't be collected by GC either
//since they can be deallocated after the function scope is over

//Just cebra objects.  What about ALLOCATE_CHAR???
//That should be owned by the string object
//so calling FREE_OBJ should take care of that

//so we have two types of memory we need to track
//  c program memory
//  cebra vm memory (a subset of c program memory)

//vm memory
#define ALLOCATE_CHAR(len) ((char*)realloc(NULL, sizeof(char) * (len)))
#define ALLOCATE_OBJ(type) ((type*)realloc(NULL, sizeof(type)))
//#define FREE_OBJ(object) free_object(object)

//compiler memory
#define ALLOCATE_NODE(type) ((type*)realloc(NULL, sizeof(type)))
#define ALLOCATE_ARRAY(type) ((type*)realloc(NULL, 0))
#define GROW_ARRAY(type, ptr, new_capacity) ((type*)realloc(ptr, sizeof(type) * new_capacity))
#define FREE(ptr) (free(ptr))
/*
void free_object(Obj* obj) {
    switch(obj->type) {
        case OBJ_STRING:
            ObjString* obj_string = (ObjString*)obj;
            FREE(obj_string->chars);
            FREE(obj_string);
            break;
        case OBJ_FUNCTION:
            break;
    }
}

typedef struct {
    int compiler_bytes;
    int vm_bytes;
} MemoryManager;

MemoryManage mm;*/


#endif// CEBRA_MEMORY_H
