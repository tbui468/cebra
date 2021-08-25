#include <string.h>
#include <stdio.h>

#include "table.h"
#include "memory.h"

#define MAX_LOAD 0.75

static void reset_table_capacity(struct Table* table, int capacity) {
    table->pairs = GROW_ARRAY(table->pairs, struct Pair, capacity, table->capacity);
    table->capacity = capacity;
    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        struct Pair pair;
        pair.key = NULL;
        pair.value = to_nil();
        table->pairs[i] = pair;
    }
}

void init_table(struct Table* table) {
    table->pairs = ALLOCATE_ARRAY(struct Pair);
    table->count = 0;
    table->capacity = 0;
    reset_table_capacity(table, 8);
}

void free_table(struct Table* table) {
    FREE_ARRAY(table->pairs, struct Pair, table->capacity);
}

static struct Table copy_table(struct Table* table) {
    struct Table copy;
    init_table(&copy);
    reset_table_capacity(&copy, table->capacity); 

    for (int i = 0; i < table->capacity; i++) {
        struct Pair* pair = &table->pairs[i];
        if (pair->key != NULL) {
            set_pair(&copy, pair->key, pair->value);
        }
    }

    return copy;
}

static void grow_table(struct Table* table) {
    int new_capacity = table->capacity * 2;
    struct Table original = copy_table(table);
    reset_table_capacity(table, new_capacity);

    for (int i = 0; i < original.capacity; i++) {
        struct Pair* pair = &original.pairs[i];
        if (pair->key != NULL) {
            set_pair(table, pair->key, pair->value);
        }
    }

    free_table(&original);
}

static bool same_keys(struct ObjString* key1, struct ObjString* key2) {
    return  key1->hash == key2->hash &&
            key1->length == key2->length &&
            memcmp(key1->chars, key2->chars, key2->length) == 0;
}

void set_pair(struct Table* table, struct ObjString* key, Value value) {
    int idx = key->hash % table->capacity;
    for (int i = idx; i < table->capacity + idx; i++) {
        int mod_i = i % table->capacity;
        struct Pair* pair = &table->pairs[mod_i];

        if (pair->key == NULL) {
            if (table->capacity * MAX_LOAD < table->count + 1) {
                grow_table(table);
            }
            struct Pair new_pair;
            new_pair.key = key;
            new_pair.value = value;
            table->pairs[mod_i] = new_pair;
            table->count++;
            return;
        }

        if (same_keys(pair->key, key)) {
            pair->value = value;
            return;
        }
    }
}


bool get_value(struct Table* table, struct ObjString* key, Value* value) {
    int idx = key->hash % table->capacity;
    for (int i = idx; i < table->capacity + idx; i++) {
        int mod_i = i % table->capacity;
        struct Pair* pair = &table->pairs[mod_i];
        if (pair->key == NULL) {
            return false;
        }

        if (same_keys(pair->key, key)) {
            *value = pair->value;
            return true;
        }
    }

    return false;
}


void print_table(struct Table* table) {
    printf("Table count: %d, Table capacity: %d \n", table->count, table->capacity);
    for (int i = 0; i < table->capacity; i++) {
        struct Pair* pair = &table->pairs[i];
        if (pair->key != NULL) {
            printf("Key: %.*s, Value: ", pair->key->length, pair->key->chars);
            print_value(pair->value);
            printf("\n");
        } else {
            printf("NULL\n");
        }
    }
}


