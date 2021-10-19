#ifndef CEBRA_VM_H
#define CEBRA_VM_H

#include "result_code.h"
#include "value.h"
#include "compiler.h"

typedef struct {
    const char* message;
} RuntimeError;

typedef struct {
    struct ObjFunction* function;
    Value* locals;
    int ip;
    int arity;
} CallFrame;

typedef struct {
    Value stack[1024]; //don't want to use my realloc to allocate memory since this is GC safeplace 
    Value* stack_top;
    CallFrame frames[128];
    int frame_count;
    struct ObjUpvalue* open_upvalues;
    RuntimeError errors[128];
    int error_count;
    struct Table globals;
    struct Table strings;
    bool initialized;
} VM;

ResultCode init_vm(VM* vm);
ResultCode free_vm(VM* vm);
ResultCode run(VM* vm, struct ObjFunction* script);
Value pop(VM* vm);
void pop_stack(VM* vm);
void push(VM* vm, Value value);
void print_stack(VM* vm);

#endif// CEBRA_VM_H
