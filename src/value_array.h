#ifndef CEBRA_VALUE_ARRAY_H
#define CEBRA_VALUE_ARRAY_H

#include "value.h"

struct ValueArray {
    Value* values;
    int count;
    int capacity;
};

void init_value_array(struct ValueArray* va);
int free_value_array(struct ValueArray* va);
int add_value(struct ValueArray* va, Value value);
void copy_value_array(struct ValueArray* dest, struct ValueArray* src);

#endif// CEBRA_VALUE_ARRAY_H
