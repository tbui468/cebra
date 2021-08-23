#ifndef CEBRA_OBJECT_H
#define CEBRA_OBJECT_H

#include <stdint.h>
#include "chunk.h"

struct Table;

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

typedef struct {
    Obj base;
    char* chars;
    int length;
    uint32_t hash;
} ObjString;

typedef struct {
    Obj base;
    int arity;
    Chunk chunk;
} ObjFunction;

typedef struct {
    Obj base;
    Chunk chunk;
} ObjClass;

typedef struct {
    Obj base;
    struct Table* props; //both for both data and methods
} ObjInstance;


ObjString* make_string(const char* start, int length);
ObjString* take_string(char* start, int length);
ObjFunction* make_function(Chunk chunk, int arity);
ObjClass* make_class(Chunk chunk);
ObjInstance* make_instance(struct Table* table);
void free_object(Obj* obj);


#endif// CEBRA_OBJECT_H
