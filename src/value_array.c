#include "value_array.h"
#include "memory.h"


void init_value_array(struct ValueArray* va) {
    va->count = 0; 
    va->capacity = 0;
    va->values = ALLOCATE_ARRAY(Value);
}

//Only freeing the array - GC needs to take are of Objects
void free_value_array(struct ValueArray* va) {
    FREE_ARRAY(va->values, Value, va->capacity);
}

void add_value(struct ValueArray* va, Value value) {
    if (va->count + 1 > va->capacity) {
        int new_capacity = va->capacity == 0 ? 8 : va->capacity * 2;
        va->values = GROW_ARRAY(va->values, Value, new_capacity, va->capacity);
        va->capacity = new_capacity;
    }

    va->values[va->count] = value;
    va->count++;
}
