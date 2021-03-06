#ifndef CEBRA_MEMORY_H
#define CEBRA_MEMORY_H

#include <stdlib.h>
#include "obj.h"
#include "vm.h"

#define ALLOCATE(type) ((type*)realloc_mem(NULL, sizeof(type), 0))
#define ALLOCATE_ARRAY(type) ((type*)realloc_mem(NULL, 0, 0))
#define GROW_ARRAY(ptr, type, new_count, old_count) \
    ((type*)realloc_mem((void*)ptr, sizeof(type) * new_count, sizeof(type) * old_count))

#define FREE(ptr, type) (free_mem((void*)ptr, sizeof(type)))
#define FREE_ARRAY(ptr, type, count) (free_mem((void*)ptr, sizeof(type) * count))

void* realloc_mem(void* ptr, size_t new_size, size_t old_size);
int free_mem(void* ptr, size_t size);

void init_memory_manager();
void free_memory_manager();
void print_memory();
void collect_garbage();

void push_root(Value value);
Value pop_root();

void push_gray(struct Obj* object);
struct Obj* pop_gray();

typedef struct {
    int allocated;
    int next_gc;
    struct Obj* objects;
    VM* vm;
    struct Obj** grays;
    int gray_capacity;
    int gray_count;
} MemoryManager;

extern MemoryManager mm;

#endif// CEBRA_MEMORY_H
