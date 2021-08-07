#ifndef CEBRA_VALUE_H
#define CEBRA_VALUE_H

typedef enum {
    VAL_INT,
    VAL_FLOAT,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int integer_type;
        double float_type;
    } as;
} Value;


Value to_float(double num);
Value to_integer(int num);
Value negate_value(Value value);
Value add_values(Value a, Value b);
Value subtract_values(Value a, Value b);
Value multiply_values(Value a, Value b);
Value divide_values(Value a, Value b);




#endif// CEBRA_VALUE_H

