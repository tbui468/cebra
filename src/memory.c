#include <stdio.h>
#include "memory.h"

MemoryManager mm;

//make a memory tracker here
void* allocate_node(size_t size) {
    mm.node_bytes_allocated += size;
    return realloc(NULL, size);
}

ValueType* allocate_value_type(int len) {
    mm.compiler_bytes_allocated += sizeof(ValueType) * len;
    return (ValueType*)realloc(NULL, sizeof(ValueType) * len);
}

void* allocate_obj(size_t size) {
    mm.object_bytes_allocated += size;
    return realloc(NULL, size);
}

void* allocate_char(size_t size) {
    mm.char_bytes_allocated += size;
    return realloc(NULL, size);
}

void init_memory_manager() {
    mm.node_bytes_allocated = 0;
    mm.node_bytes_freed = 0;
    mm.array_bytes_allocated = 0;
    mm.array_bytes_freed = 0;
    mm.object_bytes_allocated = 0;
    mm.object_bytes_freed = 0;
    mm.char_bytes_allocated = 0;
    mm.char_bytes_freed = 0;
    mm.compiler_bytes_allocated = 0;
    mm.compiler_bytes_freed = 0;
    mm.objects = NULL;
}

void print_memory() {
    printf("node bytes allocated: %d\n", mm.node_bytes_allocated);
    printf("node bytes freed: %d\n", mm.node_bytes_freed);
    printf("array bytes allocated bytes: %d\n", mm.array_bytes_allocated);
    printf("array bytes freed bytes: %d\n", mm.array_bytes_freed);
    printf("object bytes allocated bytes: %d\n", mm.object_bytes_allocated);
    printf("object bytes freed bytes: %d\n", mm.object_bytes_freed);
    printf("char bytes allocated bytes: %d\n", mm.char_bytes_allocated);
    printf("char bytes freed bytes: %d\n", mm.char_bytes_freed);
    printf("compiler bytes allocated bytes: %d\n", mm.compiler_bytes_allocated);
    printf("compiler bytes freed bytes: %d\n", mm.compiler_bytes_freed);
}


void free_objects() {
    Obj* obj = mm.objects;
    while (obj != NULL) {
        Obj* current = obj;
        obj = obj->next;
        free_object(current);
    }
    mm.objects = NULL;
}
