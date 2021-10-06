#ifndef CEBRA_CHUNK_H
#define CEBRA_CHUNK_H

#include "value_array.h"

typedef enum {
    OP_CONSTANT,
    OP_FUN,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_LESS,
    OP_GREATER,
    OP_EQUAL,
    OP_GET_PROP,
    OP_SET_PROP,
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    OP_SET_UPVALUE,
    OP_GET_UPVALUE,
    OP_CLOSE_UPVALUE,
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
    OP_ADD_PROP,
    OP_INSTANCE,
    OP_RETURN,
    OP_NATIVE,
    OP_LIST,
    OP_GET_SIZE,
    OP_SET_SIZE,
    OP_GET_ELEMENT,
    OP_SET_ELEMENT,
    OP_IN_LIST,
    OP_MAP,
    OP_GET_KEYS,
    OP_GET_VALUES,
    OP_CAST,
    OP_ENUM,
    OP_ADD_GLOBAL,
    OP_GET_GLOBAL
} OpCode;

typedef struct {
    OpCode* codes;
    int count; 
    int capacity;
    struct ValueArray constants;
} Chunk;

void init_chunk(Chunk* chunk);
int free_chunk(Chunk* chunk);
void disassemble_chunk(struct ObjFunction* function);

#endif// CEBRA_CHUNK_H
