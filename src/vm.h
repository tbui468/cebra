#ifndef CEBRA_VM_H
#define CEBRA_VM_H

#include "result_code.h"
#include "value.h"
#include "compiler.h"
#include "node_list.h"

typedef struct {
    struct ObjFunction* function;
    Value* locals;
    int ip;
    int arity;
} CallFrame;

typedef struct {
    Value stack[256];
    Value* stack_top;
    CallFrame frames[256];
    int frame_count;
    struct ObjUpvalue* open_upvalues;
} VM;

ResultCode init_vm(VM* vm);
ResultCode free_vm(VM* vm);
ResultCode run(VM* vm, struct ObjFunction* script);


Value pop(VM* vm);
void push(VM* vm, Value value);

#endif// CEBRA_VM_H
