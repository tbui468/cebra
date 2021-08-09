#include <string.h>

#include "value.h"
#include "memory.h"


Value to_float(double num) {
    Value value;
    value.type = VAL_FLOAT;
    value.as.float_type = num;
    return value;
}

Value to_integer(int32_t num) {
    Value value;
    value.type = VAL_INT;
    value.as.integer_type = num;
    return value;
}

Value to_string(ObjString* obj) {
    Value value;
    value.type = VAL_STRING;
    value.as.string_type = obj;
    return value;
}

Value negate_value(Value value) {
    if (value.type == VAL_INT) {
        return to_integer(-value.as.integer_type);
    } else {
        return to_float(-value.as.float_type);
    }
}

Value add_values(Value a, Value b) {
    if (b.type == VAL_INT) {
        return to_integer(a.as.integer_type + b.as.integer_type);
    } else if (b.type == VAL_FLOAT) {
        return to_float(a.as.float_type + b.as.float_type);
    } else if (b.type == VAL_STRING) {
        ObjString* left = a.as.string_type;
        ObjString* right = b.as.string_type;

        int length =  left->length + right->length;
        char* concat = ALLOCATE(char, length + 1);
        memcpy(concat, left->chars, left->length);
        memcpy(concat + left->length, right->chars, right->length);
        concat[length] = '\0';

        ObjString* obj = take_string(concat, length);
        return to_string(obj);
    }
}
Value subtract_values(Value a, Value b) {
    if (b.type == VAL_INT) {
        return to_integer(a.as.integer_type - b.as.integer_type);
    } else {
        return to_float(a.as.float_type - b.as.float_type);
    }
}
Value multiply_values(Value a, Value b) {
    if (b.type == VAL_INT) {
        return to_integer(a.as.integer_type * b.as.integer_type);
    } else {
        return to_float(a.as.float_type * b.as.float_type);
    }
}
Value divide_values(Value a, Value b) {
    if (b.type == VAL_INT) {
        return to_integer(a.as.integer_type / b.as.integer_type);
    } else {
        return to_float(a.as.float_type / b.as.float_type);
    }
}
