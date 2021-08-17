#ifndef CEBRA_VALUE_H
#define CEBRA_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include "object.h"
#include "token.h"

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_STRING,
    VAL_FUNCTION,
    VAL_NIL,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int32_t integer_type;
        double float_type;
        ObjString* string_type;
        bool boolean_type;
        ObjFunction* function_type;
    } as;
} Value;

typedef struct {
    ValueType* types;
    int count;
    int capacity;
} SigList;


void init_sig_list(SigList* sl);
void free_sig_list(SigList* sl);
void add_sig_type(SigList* sl, ValueType type);
SigList copy_sig_list(SigList* sl);

Value to_float(double num);
Value to_integer(int32_t num);
Value to_string(ObjString* obj);
Value to_boolean(bool b);
Value to_function(ObjFunction* obj);
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
ValueType get_value_type(Token token);

#endif// CEBRA_VALUE_H

