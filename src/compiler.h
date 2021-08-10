#ifndef CEBRA_COMPILER_H
#define CEBRA_COMPILER_H

#include "result_code.h"
#include "ast.h"
#include "value.h"
#include "decl_list.h"

typedef struct {
    Token token;
    const char* message;
} CompileError;

typedef enum {
    OP_INT,
    OP_FLOAT,
    OP_STRING,
    OP_PRINT,
    //No OP_DECL_VAR since we're using stack based local variables
    OP_TRUE,
    OP_FALSE,
    OP_SET_VAR,
    OP_GET_VAR,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_POP,
    OP_RETURN
} OpCode;

typedef struct {
    OpCode* codes;
    int count; 
    int capacity;
} Chunk;

typedef struct {
    Token name;
    int depth;
} Local;

typedef struct {
    Chunk* chunk;
    int scope_depth;
    Local locals[256];
    int locals_count;
    CompileError errors[256];
    int error_count;
} Compiler;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
ResultCode compile(DeclList* dl, Chunk* chunk);
void disassemble_chunk(Chunk* chunk);
const char* op_to_string(OpCode op);

#endif// CEBRA_COMPILER_H
