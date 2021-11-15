#ifndef CEBRA_VALUE_H
#define CEBRA_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include "token.h"

struct ObjInstance;
struct ObjFunction;
struct ObjStruct;
struct ObjString;
struct ObjNative;
struct ObjList;
struct ObjMap;
struct ObjEnum;
struct ObjFile;

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_BYTE,
    VAL_STRING,
    VAL_FUNCTION,
    VAL_STRUCT,
    VAL_INSTANCE,
    VAL_NIL,
    VAL_TYPE,
    VAL_NATIVE,
    VAL_LIST,
    VAL_MAP,
    VAL_ENUM,
    VAL_FILE
} ValueType;

typedef struct {
    ValueType type;
    union {
        int32_t integer_type;
        double float_type;
        bool boolean_type;
        uint8_t byte_type;
        struct ObjString* string_type;
        struct ObjFunction* function_type;
        struct ObjStruct* class_type;
        struct ObjInstance* instance_type;
        struct Type* type_type;
        struct ObjNative* native_type;
        struct ObjList* list_type;
        struct ObjMap* map_type;
        struct ObjEnum* enum_type;
        struct ObjFile* file_type;
    } as;
} Value;


Value to_float(double num);
Value to_integer(int32_t num);
Value to_string(struct ObjString* obj);
Value to_boolean(bool b);
Value to_byte(uint8_t byte);
Value to_function(struct ObjFunction* obj);
Value to_struct(struct ObjStruct* obj);
Value to_instance(struct ObjInstance* obj);
Value to_type(struct Type* type);
Value to_native(struct ObjNative* obj);
Value to_list(struct ObjList* obj);
Value to_map(struct ObjMap* obj);
Value to_enum(struct ObjEnum* obj);
Value to_file(struct ObjFile* obj);
Value to_nil();
Value subtract_values(Value a, Value b);
Value multiply_values(Value a, Value b);
Value divide_values(Value a, Value b);
Value less_values(Value a, Value b);
Value greater_values(Value a, Value b);
Value mod_values(Value a, Value b);
Value equal_values(Value a, Value b);
Value cast_primitive(ValueType to_type, Value* value);
void print_value(Value a);
const char* value_type_to_string(ValueType type);
struct Obj* get_object(Value* value);
Value copy_value(Value* value);


struct ValueArray {
    Value* values;
    int count;
    int capacity;
};

void init_value_array(struct ValueArray* va);
int free_value_array(struct ValueArray* va);
int add_value(struct ValueArray* va, Value value);
void copy_value_array(struct ValueArray* dest, struct ValueArray* src);


#endif// CEBRA_VALUE_H

