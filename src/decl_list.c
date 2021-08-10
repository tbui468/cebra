#include <stdio.h>
#include "decl_list.h"
#include "memory.h"

void free_node(struct Node*);
void print_node(struct Node*);

void init_decl_list(DeclList* dl) {
    dl->decls = ALLOCATE_ARRAY(struct Node*);
    dl->count = 0;
    dl->capacity = 0;
}

void free_decl_list(DeclList* dl) {
    for (int i = 0; i < dl->count; i++) {
        free_node(dl->decls[i]);
    }
    FREE(dl->decls);
}

void add_decl(DeclList* dl, struct Node* node) {
    if (dl->count + 1 > dl->capacity) {
        int new_capacity = dl->capacity == 0 ? 8 : dl->capacity * 2;
        dl->decls = GROW_ARRAY(struct Node*, dl->decls, new_capacity);
        dl->capacity = new_capacity;
    }

    dl->decls[dl->count] = node;
    dl->count++;
}


void print_decl_list(DeclList* dl) {
    for (int i = 0; i < dl->count; i++) {
        print_node(dl->decls[i]);
        printf("\n");
    }
}
