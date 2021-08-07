#ifndef CEBRA_MEMORY_H
#define CEBRA_MEMORY_H

//for ast nodes
#define ALLOCATE(type, size) ((type*)malloc(size))

//for arrays
#define ALLOCATE_ARRAY(type) ((type*)realloc(NULL, 0))
#define GROW_ARRAY(type, ptr, new_capacity) ((type*)realloc(ptr, sizeof(type) * new_capacity))

#define FREE(ptr) (free(ptr))

#endif// CEBRA_MEMORY_H
