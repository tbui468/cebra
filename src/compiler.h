#ifndef CEBRA_COMPILER_H
#define CEBRA_COMPILER_H

#include "result_code.h"
#include "ast.h"
#include "chunk.h"
#include "value.h"
#include "error.h"

struct Upvalue {
    int index;
    bool is_local;
};

typedef struct {
    Token name;
    struct Type* type;
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
    struct Error* errors;
    int error_count;
    struct Compiler* enclosing;
    struct Type* types;
    struct Node* nodes;
    struct Table globals;
    struct TypeArray* return_types;
};

extern struct Compiler* current_compiler;
extern struct Compiler* script_compiler;

void init_compiler(struct Compiler* compiler, const char* start, int length, int parameter_count, Token name, struct Type* type);
void free_compiler(struct Compiler* compiler);
ResultCode compile_script(struct Compiler* compiler, struct NodeList* nl);
const char* op_to_string(OpCode op);
void print_locals(struct Compiler* compiler);
ResultCode define_native(struct Compiler* compiler, const char* name, ResultCode (*function)(Value*, struct ValueArray*), struct Type* type);

#endif// CEBRA_COMPILER_H
