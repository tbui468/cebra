#include <stdio.h>
#include "node_list.h"
#include "memory.h"

void free_node(struct Node*);
void print_node(struct Node*);

void init_node_list(NodeList* nl) {
    nl->nodes = ALLOCATE_ARRAY(struct Node*);
    nl->count = 0;
    nl->capacity = 0;
}

void free_node_list(NodeList* nl) {
    for (int i = 0; i < nl->count; i++) {
        free_node(nl->nodes[i]);
    }
    FREE_ARRAY(nl->nodes, struct Node**, nl->capacity);
}

void add_node(NodeList* nl, struct Node* node) {
    if (nl->count + 1 > nl->capacity) {
        int new_capacity = nl->capacity == 0 ? 8 : nl->capacity * 2;
        nl->nodes = GROW_ARRAY(nl->nodes, struct Node*, new_capacity, nl->capacity);
        nl->capacity = new_capacity;
    }

    nl->nodes[nl->count] = node;
    nl->count++;
}


void print_node_list(NodeList* nl) {
    for (int i = 0; i < nl->count; i++) {
        print_node(nl->nodes[i]);
        printf("\n");
    }
}
