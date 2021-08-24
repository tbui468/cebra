#ifndef CEBRA_TABLE_H
#define CEBRA_TABLE_H

#include "object.h"
#include "value.h"
#include "memory.h"

typedef struct {
    ObjString* key;
    Value value;
} Pair;

struct Table {
    Pair* pairs;
    int count;
    int capacity;
};

void init_table(struct Table* table);
void set_pair(struct Table* table, ObjString* key, Value value);
bool get_value(struct Table* table, ObjString* key, Value* value);
//void delete_pair(struct Table* table, ObjString* key);
void free_table(struct Table* table);
void print_table(struct Table* table);

#endif // CEBRA_TABLE_H
