#ifndef CEBRA_VM_H
#define CEBRA_VM_H

#include "result_code.h"
#include "value.h"
#include "compiler.h"

typedef struct {
    Value stack[256];
    int stack_top;
    int ip;
} VM;

ResultCode vm_init(VM* vm);
ResultCode vm_free(VM* vm);
ResultCode execute(VM* vm, Chunk* chunk);

#endif// CEBRA_VM_H
