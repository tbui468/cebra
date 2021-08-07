#ifndef CEBRA_COMPILER_H
#define CEBRA_COMPILER_H

#include "result_code.h"
#include "ast.h"

typedef enum {
    OP_INT, //two bytes
    OP_FLOAT, //two bytes
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
    int integers[256];
    int integers_idx;
    double floats[256];
    int floats_idx;
} Chunk;

typedef struct {
    Chunk* chunk;
    Expr* ast;
} Compiler;

void chunk_init(Chunk* chunk);
ResultCode compile(Expr* ast, Chunk* chunk);
void disassemble_chunk(Chunk* chunk);

#endif// CEBRA_COMPILER_H
