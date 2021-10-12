#ifndef CEBRA_TABLE_H
#define CEBRA_TABLE_H

#include "value.h"

struct ObjString;

struct Pair {
    struct ObjString* key;
    Value value;
};

struct Table {
    struct Pair* pairs;
    int count;
    int capacity;
};

void init_table(struct Table* table);
void set_table(struct Table* table, struct ObjString* key, Value value);
bool get_from_table(struct Table* table, struct ObjString* key, Value* value);
bool same_keys(struct ObjString* key1, struct ObjString* key2);
//void delete_pair(struct Table* table, ObjString* key); //TODO: not implemented yet
void copy_table(struct Table* dest, struct Table* src);
int free_table(struct Table* table);
void print_table(struct Table* table);
struct ObjString* find_interned_string(struct Table* table, const char* chars, int length, uint32_t hash);

#endif // CEBRA_TABLE_H
