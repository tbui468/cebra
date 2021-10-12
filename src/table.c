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
        table->pairs[i].key = NULL;
        table->pairs[i].value = to_nil();
    }
}

void init_table(struct Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->pairs = ALLOCATE_ARRAY(struct Pair);
}

int free_table(struct Table* table) {
    return FREE_ARRAY(table->pairs, struct Pair, table->capacity);
}

void copy_table(struct Table* dest, struct Table* src) {
    reset_table_capacity(dest, src->capacity); 
    for (int i = 0; i < src->capacity; i++) {
        struct Pair* pair = &src->pairs[i];
        if (pair->key != NULL) {
            push_root(pair->value);
            Value copy = copy_value(&pair->value);
            push_root(copy);
            set_table(dest, pair->key, copy);
            pop_root();
            pop_root();
        }
    }
}

static void grow_table(struct Table* table) {

    //make an ObjEnum and pushing onto stack so that
    //GC doesn't sweep temporary table used to grow table
    struct ObjEnum* temp_table = make_enum(make_dummy_token());
    push_root(to_enum(temp_table));
    copy_table(&temp_table->props, table);

    int new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    reset_table_capacity(table, new_capacity);

    for (int i = 0; i < temp_table->props.capacity; i++) {
        struct Pair* pair = &temp_table->props.pairs[i];
        if (pair->key != NULL) {
            set_table(table, pair->key, pair->value);
        }
    }

    pop_root();
}


bool same_keys(struct ObjString* key1, struct ObjString* key2) {
    return  key1->hash == key2->hash &&
            key1->length == key2->length &&
            memcmp(key1->chars, key2->chars, key2->length) == 0;
}

void set_table(struct Table* table, struct ObjString* key, Value value) {
    Value v;
    if (!get_from_table(table, key, &v) && table->capacity * MAX_LOAD < table->count + 1) {
        grow_table(table);
    }

    int idx = key->hash % table->capacity;
    for (int i = idx; i < table->capacity + idx; i++) {
        int mod_i = i % table->capacity;
        struct Pair* pair = &table->pairs[mod_i];

        if (pair->key == NULL) {
            table->pairs[mod_i].key = key;
            table->pairs[mod_i].value = value;
            table->count++;
            return;
        }

        if (same_keys(pair->key, key)) {
            pair->value = value;
            return;
        }
    }
}


bool get_from_table(struct Table* table, struct ObjString* key, Value* value) {
    if (table->capacity == 0) return false;

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


