#ifndef CEBRA_VALUE_ARRAY_H
#define CEBRA_VALUE_ARRAY_H

#include "value.h"

struct ValueArray {
    Value* values;
    int count;
    int capacity;
};

void init_value_array(struct ValueArray* va);
void free_value_array(struct ValueArray* va);
void add_value(struct ValueArray* va, Value value);

#endif// CEBRA_VALUE_ARRAY_H
