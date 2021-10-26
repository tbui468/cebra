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
    int pushed = 0;
    for (int i = 0; i < src->capacity; i++) {
        struct Pair* pair = &src->pairs[i];
        if (pair->key == NULL) continue;

        Value copy = copy_value(&pair->value);
        push_root(copy);
        pushed++;
        set_table(dest, pair->key, copy);
    }

    for (int i = 0; i < pushed; i++) {
        pop_root();
    }
}

static void grow_table(struct Table* table) {
    struct Table temp;
    init_table(&temp);
    copy_table(&temp, table);
    int pushed_count = 0;
    for (int i = 0; i < temp.capacity; i++) {
        struct Pair* pair = &temp.pairs[i];
        if (pair->key == NULL) continue;
        push_root(to_string(pair->key));
        push_root(pair->value);
        pushed_count += 2;
    }


    int new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    reset_table_capacity(table, new_capacity);

    for (int i = 0; i < temp.capacity; i++) {
        struct Pair* pair = &temp.pairs[i];
        if (pair->key == NULL) continue;
        set_table(table, pair->key, pair->value);
    }

    free_table(&temp);

    for (int i = 0; i < pushed_count; i++) {
        pop_root();
    }
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

    int first_tombstone = -1;

    int idx = key->hash % table->capacity;
    for (;;) {
        struct Pair* pair = &table->pairs[idx];

        if (pair->key == NULL && pair->value.type == VAL_NIL) {
            if (first_tombstone != -1) {
                table->pairs[first_tombstone].key = key;
                table->pairs[first_tombstone].value = value;
            } else {
                table->pairs[idx].key = key;
                table->pairs[idx].value = value;
            }
            table->count++;
            return;
        }

        if (pair->key == NULL && pair->value.type == VAL_BOOL && pair->value.as.boolean_type && first_tombstone == -1) {
            first_tombstone = idx;
        }

        if (pair->key != NULL && same_keys(pair->key, key)) {
            pair->value = value;
            return;
        }

        idx = (idx + 1) % table->capacity;
    }
}


bool get_from_table(struct Table* table, struct ObjString* key, Value* value) {
    if (table->capacity == 0) return false;

    int idx = key->hash % table->capacity;
    for (;;) {
        struct Pair* pair = &table->pairs[idx];
        if (pair->key == NULL && pair->value.type == VAL_NIL) {
            return false;
        }

        //check if pair->key is NULL to skip tombstones
        if (pair->key != NULL && same_keys(pair->key, key)) {
            *value = pair->value;
            return true;
        }

        idx = (idx + 1) % table->capacity;
    }

}

struct ObjString* find_interned_string(struct Table* table, const char* chars, int length, uint32_t hash) {
    if (table->capacity == 0) return false;

    int idx = hash % table->capacity;
    for (;;) {
        struct Pair* pair = &table->pairs[idx];
        if (pair->key == NULL && pair->value.type == VAL_NIL) {
            return NULL;
        }

        if (pair->key != NULL &&
            pair->key->hash == hash &&
            pair->key->length == length &&
            memcmp(pair->key->chars, chars, length) == 0) {
            return pair->key;
        }

        idx = (idx + 1) % table->capacity;
    }

}


void delete_entry(struct Table* table, struct ObjString* key) {
    int idx = key->hash % table->capacity;
    for (;;) {
        struct Pair* pair = &table->pairs[idx];
        if (pair->key == NULL && pair->value.type == VAL_NIL) {
            return;
        }


        if (pair->key != NULL && same_keys(pair->key, key)) {
            //tombstone is 'NULL' key and 'true' value
            table->pairs[idx].key = NULL;
            table->pairs[idx].value = to_boolean(true);
            return;
        }

        idx = (idx + 1) % table->capacity;
    }

}

void print_table(struct Table* table) {
    printf("Table count: %d, Table capacity: %d \n", table->count, table->capacity);
    for (int i = 0; i < table->capacity; i++) {
        struct Pair* pair = &table->pairs[i];
        printf("[%d] ", i);
        if (pair->key != NULL) {
            printf("Key: %.*s, Value: ", pair->key->length, pair->key->chars);
            print_value(pair->value);
            printf("\n");
        } else {
            if (pair->value.type == VAL_BOOL && pair->value.as.boolean_type) {
                printf("tombstone\n");
            } else {
                printf("NULL\n");
            }
        }
    }
}


