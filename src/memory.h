#ifndef CEBRA_MEMORY_H
#define CEBRA_MEMORY_H

#include <stdlib.h>

//for strings in object.c and value.c
#define ALLOCATE_CHAR(len) ((char*)realloc(NULL, sizeof(char) * (len)))

//for objects
#define ALLOCATE_OBJ(type) ((type*)realloc(NULL, sizeof(type)))

//for ast nodes
#define ALLOCATE_NODE(type) ((type*)realloc(NULL, sizeof(type)))

//for arrays
#define ALLOCATE_ARRAY(type) ((type*)realloc(NULL, 0))
#define GROW_ARRAY(type, ptr, new_capacity) ((type*)realloc(ptr, sizeof(type) * new_capacity))

#define FREE(ptr) (free(ptr))

#endif// CEBRA_MEMORY_H
