#ifndef CEBRA_VALUE_H
#define CEBRA_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include "token.h"

#define IS_NIL(value) (value.type == VAL_NIL)

struct ObjInstance;
struct ObjFunction;
struct ObjClass;
struct ObjString;
struct Sig;

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_STRING,
    VAL_FUNCTION,
    VAL_CLASS,
    VAL_INSTANCE,
    VAL_SIG,
    VAL_NIL,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int32_t integer_type;
        double float_type;
        struct ObjString* string_type;
        bool boolean_type;
        struct ObjFunction* function_type;
        struct ObjClass* class_type;
        struct ObjInstance* instance_type;
        struct Sig* sig_type;
    } as;
} Value;


Value to_float(double num);
Value to_integer(int32_t num);
Value to_string(struct ObjString* obj);
Value to_boolean(bool b);
Value to_function(struct ObjFunction* obj);
Value to_class(struct ObjClass* obj);
Value to_instance(struct ObjInstance* obj);
Value to_sig(struct Sig* sig);
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
ValueType get_value_type(Token token);

#endif// CEBRA_VALUE_H

