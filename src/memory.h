#ifndef CEBRA_MEMORY_H
#define CEBRA_MEMORY_H

#define ALLOCATE(type, size) ((type*)malloc(size))

#define FREE(ptr) (free(ptr))

#endif// CEBRA_MEMORY_H
