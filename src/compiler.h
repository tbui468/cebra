#ifndef CEBRA_COMPILER_H
#define CEBRA_COMPILER_H

#include "result_code.h"
#include "ast.h"
#include "decl_list.h"

typedef struct {
    Token token;
    const char* message;
} CompileError;

typedef enum {
    OP_INT,
    OP_FLOAT,
    OP_STRING,
    OP_FUN,
    OP_PRINT,
    //No OP_DECL_VAR since we're using stack based local variables
    OP_TRUE,
    OP_FALSE,
    OP_LESS,
    OP_GREATER,
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
    Chunk chunk;
    int scope_depth;
    Local locals[256];
    int locals_count;
    CompileError errors[256];
    int error_count;
    struct Compiler* enclosing;
} Compiler;

void init_compiler(Compiler* compiler);
void free_compiler(Compiler* compiler);
ResultCode compile(Compiler* compiler, DeclList* dl);
void disassemble_chunk(Chunk* chunk);
const char* op_to_string(OpCode op);

#endif// CEBRA_COMPILER_H
