#ifndef CEBRA_OBJECT_H
#define CEBRA_OBJECT_H

#include "compiler.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjType;

typedef struct {
    ObjType type;
} Obj;

typedef struct {
    Obj base;
    char* chars;
    int length;
} ObjString;

typedef struct {
    Obj base;
    Compiler compiler;
} ObjFunction;


ObjString* make_string(const char* start, int length);
ObjString* take_string(char* start, int length);
ObjFunction* make_function(Compiler compiler);


#endif// CEBRA_OBJECT_H
