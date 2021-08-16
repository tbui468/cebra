#ifndef CEBRA_OBJECT_H
#define CEBRA_OBJECT_H

#include "chunk.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjType;

typedef struct {
    ObjType type;
    struct Obj* next;
} Obj;

typedef struct {
    Obj base;
    char* chars;
    int length;
} ObjString;

typedef struct {
    Obj base;
    int arity;
    Chunk chunk;
} ObjFunction;


ObjString* make_string(const char* start, int length);
ObjString* take_string(char* start, int length);
ObjFunction* make_function(Chunk chunk, int arity);
void free_object(Obj* obj);


#endif// CEBRA_OBJECT_H
