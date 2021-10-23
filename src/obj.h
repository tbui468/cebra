#ifndef CEBRA_OBJ_H
#define CEBRA_OBJ_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "chunk.h"
#include "obj.h"
#include "table.h"
#include "result_code.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_STRUCT,
    OBJ_INSTANCE,
    OBJ_UPVALUE,
    OBJ_NATIVE,
    OBJ_LIST,
    OBJ_MAP,
    OBJ_ENUM,
    OBJ_FILE
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
    bool is_marked;
};

struct ObjFile {
    struct Obj base;
    FILE* fp;
    struct ObjString* next_line;
    bool is_eof;
};

struct ObjString {
    struct Obj base;
    char* chars;
    int length;
    uint32_t hash;
};

struct ObjStruct {
    struct Obj base;
    struct ObjString* name;
    struct Table props;
    struct ObjStruct* super;
};

struct ObjInstance {
    struct Obj base;
    struct Table props;
    struct ObjStruct* klass;
};

struct ObjEnum {
    struct Obj base;
    struct ObjString* name;
    struct Table props;
};

struct ObjUpvalue {
    struct Obj base;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
};

struct ObjFunction {
    struct Obj base;
    struct ObjString* name;
    int arity;
    Chunk chunk;
    struct ObjUpvalue* upvalues[256];
    int upvalue_count;
};

struct ObjNative {
    struct Obj base;
    struct ObjString* name;
    ResultCode (*function)(int, Value*, struct ValueArray*);
};

struct ObjList {
    struct Obj base;
    struct ValueArray values;
    Value default_value;
};

struct ObjMap {
    struct Obj base;
    struct Table table;
    Value default_value;
};

void insert_object(struct Obj* ptr);
int free_object(struct Obj* obj);
void mark_object(struct Obj* obj);
void print_object(struct Obj* obj);

struct ObjString* make_string(const char* start, int length);
struct ObjString* take_string(char* start, int length);
struct ObjInstance* make_instance(struct Table table, struct ObjStruct* klass);
struct ObjStruct* make_struct(struct ObjString* name, struct ObjStruct* super);
struct ObjFunction* make_function(struct ObjString* name, int arity);
struct ObjUpvalue* make_upvalue(Value* location);
struct ObjNative* make_native(struct ObjString* name, ResultCode (*function)(int, Value*, struct ValueArray*));
struct ObjList* make_list();
struct ObjList* copy_list(struct ObjList* l);
struct ObjMap* make_map();
struct ObjEnum* make_enum(Token name);
struct ObjFile* make_file(FILE* fp);


#endif// CEBRA_OBJ_H

