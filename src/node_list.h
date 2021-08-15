#ifndef CEBRA_NODE_LIST_H
#define CEBRA_NODE_LIST_H

struct Node;

typedef struct {
    struct Node** nodes;
    int count;
    int capacity;
} NodeList;

void init_node_list(NodeList* nl);
void free_node_list(NodeList* nl);
void add_node(NodeList* nl, struct Node* node);
void print_node_list(NodeList* nl);

#endif// CEBRA_NODE_LIST_H
