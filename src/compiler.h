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
    struct Sig* sig;
    int depth;
} Local;

struct Compiler {
    Chunk* chunk;
    int scope_depth;
    Local locals[256];
    int locals_count;
    CompileError errors[256];
    int error_count;
    struct Compiler* enclosing;
};

void init_compiler(struct Compiler* compiler, Chunk* chunk);
ResultCode compile_ast(struct Compiler* compiler, NodeList* nl);
const char* op_to_string(OpCode op);
void print_locals(struct Compiler* compiler);

#endif// CEBRA_COMPILER_H
