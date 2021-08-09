#ifndef CEBRA_OBJECT_H
#define CEBRA_OBJECT_H

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


ObjString* make_string(const char* start, int length);


#endif// CEBRA_OBJECT_H
