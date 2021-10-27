#ifndef CEBRA_VM_H
#define CEBRA_VM_H

#include "result_code.h"
#include "value.h"
#include "compiler.h"
#include "error.h"

#define VM_STACK_MAX 1024
#define VM_FRAMES_MAX 64

typedef struct {
    struct ObjFunction* function;
    Value* locals;
    int ip;
    int arity;
} CallFrame;

typedef struct {
    Value stack[VM_STACK_MAX]; //don't want to use my realloc bc stack needs to be fast, and also this is GC safeplace
    Value* stack_top;
    CallFrame frames[VM_FRAMES_MAX];
    int frame_count;
    struct ObjUpvalue* open_upvalues;
    struct Error errors[64];
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
