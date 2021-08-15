#ifndef CEBRA_VM_H
#define CEBRA_VM_H

#include "result_code.h"
#include "value.h"
#include "compiler.h"
#include "node_list.h"

typedef struct {
    ObjFunction* function;
    int stack_offset;
    int ip;
    int arity;
} CallFrame;

typedef struct {
    Value stack[256];
    int stack_top;
    CallFrame frames[256];
    int frame_count;
} VM;

ResultCode init_vm(VM* vm);
ResultCode free_vm(VM* vm);
ResultCode compile_and_run(VM* vm, NodeList* nl);

#endif// CEBRA_VM_H
