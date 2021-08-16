#ifndef CEBRA_COMPILER_H
#define CEBRA_COMPILER_H

#include "result_code.h"
#include "ast.h"
#include "node_list.h"
#include "chunk.h"
#include "value.h"

typedef struct {
    Token token;
    const char* message;
} CompileError;


typedef struct {
    Token name;
    ValueType* types;
    int type_count;
    int depth;
} Local;

typedef struct {
    Chunk* chunk;
    int scope_depth;
    Local locals[256];
    int locals_count;
    CompileError errors[256];
    int error_count;
    struct Compiler* enclosing;
} Compiler;

void init_compiler(Compiler* compiler, Chunk* chunk);
void free_compiler(Compiler* compiler);
ResultCode compile(Compiler* compiler, NodeList* nl);
const char* op_to_string(OpCode op);
void print_locals(Compiler* compiler);

#endif// CEBRA_COMPILER_H
