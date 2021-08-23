#include <string.h>
#include <stdio.h>
#include "table.h"

static void set_table_capacity(struct Table* table, int capacity) {
    table->pairs = GROW_ARRAY(table->pairs, Pair, capacity, table->capacity);
    table->capacity = capacity;
    for (int i = 0; i < table->capacity; i++) {
        Pair pair;
        pair.key = NULL;
        pair.value = NULL;
        table->pairs[i] = pair;
    }
}

void init_table(struct Table* table) {
    table->pairs = ALLOCATE_ARRAY(Pair);
    table->count = 0;
    table->capacity = 0;
    set_table_capacity(table, 8);
}

void free_table(struct Table* table) {
    FREE_ARRAY(table->pairs, Pair, table->capacity);
}

static struct Table copy_table(struct Table* table) {
    struct Table copy;
    init_table(&copy);
    set_table_capacity(&copy, table->capacity); 

    for (int i = 0; i < table->capacity; i++) {
        Pair* pair = &table->pairs[i];
        if (pair->key != NULL && pair->value != NULL) {
            set_key_value(&copy, pair->key, pair->value);
        }
    }

    return copy;
}

static void grow_table(struct Table* table) {
    int new_capacity = table->capacity * 2;
    struct Table original = copy_table(table);
    set_table_capacity(table, new_capacity);

    for (int i = 0; i < original.capacity; i++) {
        Pair* pair = &original.pairs[i];
        if (pair->key != NULL && pair->value != NULL) {
            set_key_value(table, pair->key, pair->value);
        }
    }
}

void set_key_value(struct Table* table, ObjString* key, Value* value) {
    int idx = key->hash % table->capacity;
    for (int i = idx; i < table->capacity + idx; i++) {
        int mod_i = i % table->capacity;
        Pair* pair = &table->pairs[mod_i];

        if (pair->key == NULL && pair->value == NULL) {
            if (table->capacity * .75 < table->count + 1) {
                grow_table(table);
            }
            Pair new_pair;
            new_pair.key = key;
            new_pair.value = value;
            table->pairs[mod_i] = new_pair;
            return;
        }

        if (memcmp(&pair->key->hash, &key->hash, sizeof(uint32_t)) == 0) {
            pair->value = value;
            return;
        }
    }
}


void print_table(struct Table* table) {
    printf("Table count: %d, Table capacity: %d \n", table->count, table->capacity);
    for (int i = 0; i < table->capacity; i++) {
        Pair* pair = &table->pairs[i];
        if (pair->key != NULL && pair->value != NULL) {
            printf("Key: %.*s, Value: ", pair->key->length, pair->key->chars);
            print_value(*(pair->value));
            printf("\n");
        }
    }
}



