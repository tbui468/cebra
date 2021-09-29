#ifndef CEBRA_VALUE_H
#define CEBRA_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include "token.h"

#define IS_INT(value) (value.type == VAL_INT)
#define IS_FLOAT(value) (value.type == VAL_FLOAT)
#define IS_BOOL(value) (value.type == VAL_BOOL)
#define IS_STRING(value) (value.type == VAL_STRING)
#define IS_FUNCTION(value) (value.type == VAL_FUNCTION)
#define IS_CLASS(value) (value.type == VAL_CLASS)
#define IS_INSTANCE(value) (value.type == VAL_INSTANCE)
#define IS_NIL(value) (value.type == VAL_NIL)
#define IS_TYPE(value) (value.type == VAL_TYPE)
#define IS_NATIVE(value) (value.type == VAL_NATIVE)

struct ObjInstance;
struct ObjFunction;
struct ObjClass;
struct ObjString;
struct ObjNative;
struct ObjList;
struct ObjMap;

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_STRING,
    VAL_FUNCTION,
    VAL_CLASS,
    VAL_INSTANCE,
    VAL_NIL,
    VAL_TYPE,
    VAL_NATIVE,
    VAL_LIST,
    VAL_MAP
} ValueType;

typedef struct {
    ValueType type;
    union {
        int32_t integer_type;
        double float_type;
        bool boolean_type;
        struct ObjString* string_type;
        struct ObjFunction* function_type;
        struct ObjClass* class_type;
        struct ObjInstance* instance_type;
        struct Type* type_type;
        struct ObjNative* native_type;
        struct ObjList* list_type;
        struct ObjMap* map_type;
    } as;
} Value;


Value to_float(double num);
Value to_integer(int32_t num);
Value to_string(struct ObjString* obj);
Value to_boolean(bool b);
Value to_function(struct ObjFunction* obj);
Value to_class(struct ObjClass* obj);
Value to_instance(struct ObjInstance* obj);
Value to_type(struct Type* type);
Value to_native(struct ObjNative* obj);
Value to_list(struct ObjList* obj);
Value to_map(struct ObjMap* obj);
Value to_nil();
Value negate_value(Value value);
Value add_values(Value a, Value b);
Value subtract_values(Value a, Value b);
Value multiply_values(Value a, Value b);
Value divide_values(Value a, Value b);
Value less_values(Value a, Value b);
Value greater_values(Value a, Value b);
Value mod_values(Value a, Value b);
Value equal_values(Value a, Value b);
void print_value(Value a);
const char* value_type_to_string(ValueType type);

struct Obj* get_object(Value* value);
Value copy_value(Value* value);

#endif// CEBRA_VALUE_H

