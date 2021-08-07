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



#endif// CEBRA_VALUE_H

