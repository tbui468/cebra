#ifndef CEBRA_VM_H
#define CEBRA_VM_H

#include "result_code.h"
#include "value.h"
#include "compiler.h"
#include "decl_list.h"

typedef struct {
    Value stack[256];
    int stack_top;
    int ip;
} VM;

ResultCode init_vm(VM* vm);
ResultCode free_vm(VM* vm);
ResultCode compile_and_run(VM* vm, DeclList* dl);
ResultCode run(VM* vm, Chunk* chunk);

#endif// CEBRA_VM_H
