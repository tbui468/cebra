#ifndef CEBRA_COMPILER_H
#define CEBRA_COMPILER_H

#include "result_code.h"
#include "ast.h"
#include "chunk.h"
#include "value.h"

typedef struct {
    Token token;
    const char* message;
} CompileError;

struct Upvalue {
    int index;
    bool is_local;
};

typedef struct {
    Token name;
    struct Sig* sig;
    int depth;
    bool is_captured;
} Local;

struct Compiler {
    struct ObjFunction* function;
    int scope_depth;
    Local locals[256];
    int locals_count;
    struct Upvalue upvalues[256];
    int upvalue_count;
    CompileError errors[256];
    int error_count;
    struct Compiler* enclosing;
    struct Sig* signatures;
    struct Node* nodes;
};

extern struct Compiler* current_compiler;
extern struct Compiler* class_compiler;

void init_compiler(struct Compiler* compiler, const char* start, int length, int parameter_count);
void free_compiler(struct Compiler* compiler);
ResultCode compile_script(struct Compiler* compiler, struct NodeList* nl);
const char* op_to_string(OpCode op);
void print_locals(struct Compiler* compiler);

#endif// CEBRA_COMPILER_H
