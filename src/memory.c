#include <stdio.h>
#include "memory.h"

MemoryManager mm;


void* realloc_mem(void* ptr, size_t new_size, size_t old_size) {
    mm.bytes_allocated += (new_size - old_size);
    return realloc(ptr, new_size);
}

void free_mem(void* ptr, size_t size) {
    mm.bytes_freed += size;
    return free(ptr);
}

void init_memory_manager() {
    mm.bytes_allocated = 0;
    mm.bytes_freed = 0;
    mm.objects = NULL;
}

void print_memory() {
    printf("bytes allocated: %d\n", mm.bytes_allocated);
    printf("bytes freed: %d\n", mm.bytes_freed);
}

//TODO: remove later - only to check that all memory is being tracked and cleared
void free_objects() {
    struct Obj* obj = mm.objects;
    while (obj != NULL) {
        struct Obj* current = obj;
        obj = obj->next;
        free_object(current);
    }
    mm.objects = NULL;
}

static void mark_roots() {
    //mark Values in stack with memory allocation
    //TODO: just marking all objects for now to test if we have everything
    //      should really just be marking objects on the stack (that have heap 
    //      allocated memory) and ... other stuff...
    struct Obj* obj = mm.objects;
    while (obj != NULL) {
        obj->is_marked = true;
        obj = obj->next;
    }
}

static void sweep() {
    struct Obj* previous = NULL;
    struct Obj* current = mm.objects;
    while (current != NULL) {
        if (current->is_marked) {
            current->is_marked = false;
            previous = current;
            current = current->next;
        } else {
            if (previous == NULL) {
                struct Obj* next = current->next;
                free_object(current);
                current = next;
            } else {
                previous->next = current->next;
                free_object(current);
                current = previous->next;
            }
        }
    }
}

void collect_garbage() {
    //free_objects();
//    mark_roots();
    //trace_references();
    sweep();
}
