#ifndef CEBRA_MEMORY_H
#define CEBRA_MEMORY_H

#include <stdlib.h>
#include "object.h"

//vm memory
#define ALLOCATE_CHAR(len) ((char*)allocate_char(sizeof(char) * (len)))
#define FREE_CHAR(ptr, len) (free((char*)ptr), mm.char_bytes_freed += len)
#define ALLOCATE_OBJ(type) ((type*)allocate_obj(sizeof(type)))
#define FREE_OBJ(ptr, type) (free((type*)ptr), mm.object_bytes_freed += sizeof(type))

//AST node memory
#define ALLOCATE_NODE(type) (type*)allocate_node(sizeof(type))
#define FREE_NODE(ptr, type) (free((type*)ptr), mm.node_bytes_freed += sizeof(type))

#define ALLOCATE_ARRAY(type) ((type*)realloc(NULL, 0))
#define GROW_ARRAY(ptr, type, new_capacity, old_capacity) \
            ((mm.array_bytes_allocated += sizeof(type) * (new_capacity - old_capacity), \
             (type*)realloc(ptr, sizeof(type) * new_capacity)))
#define FREE_ARRAY(ptr, type, count) (free((type*)ptr), mm.array_bytes_freed += sizeof(type) * count)
             

void* allocate_node(size_t size);
void* allocate_obj(size_t size);
void* allocate_char(size_t size);
void init_memory_manager();
void print_memory();

//placeholder until GC in place
void free_objects();

typedef struct {
    int node_bytes_allocated;
    int node_bytes_freed;
    int array_bytes_allocated;
    int array_bytes_freed;
    int object_bytes_allocated;
    int object_bytes_freed;
    int char_bytes_allocated;
    int char_bytes_freed;
    Obj* objects;
} MemoryManager;

extern MemoryManager mm;

#endif// CEBRA_MEMORY_H
