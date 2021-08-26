#ifndef CEBRA_CHUNK_H
#define CEBRA_CHUNK_H

#include "value_array.h"

typedef enum {
    OP_INT,
    OP_FLOAT,
    OP_STRING,
    OP_FUN,
    OP_NIL,
    OP_PRINT,
    OP_TRUE,
    OP_FALSE,
    OP_LESS,
    OP_GREATER,
    OP_EQUAL,
    OP_SET_VAR,
    OP_GET_VAR,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MOD,
    OP_NEGATE,
    OP_POP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_JUMP,
    OP_JUMP_BACK,
    OP_CALL,
    OP_CLASS,
    OP_INSTANCE,
    OP_RETURN
} OpCode;

typedef struct {
    OpCode* codes;
    int count; 
    int capacity;
    struct ValueArray constants;
} Chunk;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void disassemble_chunk(Chunk* chunk);

#endif// CEBRA_CHUNK_H
