#ifndef CEBRA_TABLE_H
#define CEBRA_TABLE_H

#include "value.h"

struct ObjString;

struct Entry {
    struct ObjString* key;
    Value value;
};

struct Table {
    struct Entry* entries;
    int count;
    int capacity;
};

void init_table(struct Table* table);
void set_entry(struct Table* table, struct ObjString* key, Value value);
bool get_entry(struct Table* table, struct ObjString* key, Value* value);
void delete_entry(struct Table* table, struct ObjString* key);
void copy_table(struct Table* dest, struct Table* src);
int free_table(struct Table* table);
void print_table(struct Table* table);
struct ObjString* find_interned_string(struct Table* table, const char* chars, int length, uint32_t hash);

#endif // CEBRA_TABLE_H
