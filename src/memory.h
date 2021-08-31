#ifndef CEBRA_MEMORY_H
#define CEBRA_MEMORY_H

#include <stdlib.h>
//#include "value.h"
#include "obj.h"
#include "vm.h"

#define ALLOCATE(type) ((type*)realloc_mem(NULL, sizeof(type), 0))
#define ALLOCATE_ARRAY(type) ((type*)realloc_mem(NULL, 0, 0))
#define GROW_ARRAY(ptr, type, new_count, old_count) \
    ((type*)realloc_mem((void*)ptr, sizeof(type) * new_count, sizeof(type) * old_count))

#define FREE(ptr, type) (free_mem((void*)ptr, sizeof(type)))
#define FREE_ARRAY(ptr, type, count) (free_mem((void*)ptr, sizeof(type) * count))

void* realloc_mem(void* ptr, size_t new_size, size_t old_size);
void free_mem(void* ptr, size_t size);


void init_memory_manager(VM* vm);
void print_memory();
void collect_garbage();

//placeholder until GC in place
//this frees all objects in the mm
void free_objects();

typedef struct {
    int bytes_allocated;
    int bytes_freed;
    struct Obj* objects;
    VM* vm;
} MemoryManager;

extern MemoryManager mm;

#endif// CEBRA_MEMORY_H
