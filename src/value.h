#ifndef CEBRA_VALUE_H
#define CEBRA_VALUE_H

#include <stdint.h>
#include "object.h"

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_STRING,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int32_t integer_type;
        double float_type;
        ObjString* string_type;
    } as;
} Value;


Value to_float(double num);
Value to_integer(int32_t num);
Value to_string(ObjString* obj);
Value negate_value(Value value);
Value add_values(Value a, Value b);
Value subtract_values(Value a, Value b);
Value multiply_values(Value a, Value b);
Value divide_values(Value a, Value b);
void print_value(Value a);




#endif// CEBRA_VALUE_H

