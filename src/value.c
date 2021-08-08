#include "value.h"


Value to_float(double num) {
    Value value;
    value.type = VAL_FLOAT;
    value.as.float_type = num;
    return value;
}

Value to_integer(int num) {
    Value value;
    value.type = VAL_INT;
    value.as.integer_type = num;
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
    } else {
        return to_float(a.as.float_type + b.as.float_type);
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
