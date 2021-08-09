#ifndef CEBRA_COMPILER_H
#define CEBRA_COMPILER_H

#include "result_code.h"
#include "ast.h"
#include "value.h"
#include "decl_list.h"

typedef enum {
    OP_INT, //two bytes
    OP_FLOAT, //two bytes
    OP_PRINT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN
} OpCode;

typedef struct {
    OpCode* codes;
    int count; 
    int capacity;
    Value constants[256];
    int constants_idx;
} Chunk;

typedef struct {
    Chunk* chunk;
} Compiler;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
ResultCode compile(DeclList* dl, Chunk* chunk);
void disassemble_chunk(Chunk* chunk);

#endif// CEBRA_COMPILER_H
