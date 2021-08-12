#include "vm.h"
#include "object.h"
#include "common.h"


static Value pop(VM* vm) {
    vm->stack_top--;
    return vm->stack[vm->stack_top];
}

static Value peek(VM* vm, int depth) {
    return vm->stack[vm->stack_top - 1 - depth];
}

static void push(VM* vm, Value value) {
    vm->stack[vm->stack_top] = value;
    vm->stack_top++;
}

static uint8_t read_byte(VM* vm, Chunk* chunk) {
    vm->ip++;
    return chunk->codes[vm->ip - 1]; 
}

static uint16_t read_short(VM* vm, Chunk* chunk) {
    uint16_t sh = (uint16_t)chunk->codes[vm->ip];
    vm->ip += 2;
    return sh;
}

ResultCode init_vm(VM* vm) {
    vm->stack_top = 0;
    vm->ip = 0;
    return RESULT_SUCCESS;
}

ResultCode free_vm(VM* vm) {
    return RESULT_SUCCESS;
}


static int32_t read_int(VM* vm, Chunk* chunk) {
    int32_t* ptr = (int32_t*)(&chunk->codes[vm->ip]);
    vm->ip += (int)sizeof(int32_t);
    return *ptr;
}

static double read_float(VM* vm, Chunk* chunk) {
    double* ptr = (double*)(&chunk->codes[vm->ip]);
    vm->ip += (int)sizeof(double);
    return *ptr;
}

static ObjString* read_string(VM* vm, Chunk* chunk) {
    ObjString** ptr = (ObjString**)(&chunk->codes[vm->ip]);
    vm->ip += (int)sizeof(ObjString*);
    return *ptr;
}

static void print_trace(VM* vm, OpCode op) {
    //print opcodes - how can the compiler and this use the same code?
    printf("Op: %s\n", op_to_string(op));

    //print vm stack
    printf("Stack: ");
    for (int i = 0; i < vm->stack_top; i++) {
        printf("[ ");
        print_value(vm->stack[i]);
        printf(" ]");
    }
    printf("\n*************************\n");
}

ResultCode run(VM* vm, Chunk* chunk) {

    while (vm->ip < chunk->count) {
        uint8_t op = read_byte(vm, chunk);
        switch(op) {
            case OP_INT: {
                push(vm, to_integer(read_int(vm, chunk)));
                break;
            }
            case OP_FLOAT: {
                push(vm, to_float(read_float(vm, chunk)));
                break;
            }
            case OP_STRING: {
                push(vm, to_string(read_string(vm, chunk)));
                break;
            }
            case OP_NEGATE: {
                Value value = pop(vm);
                push(vm, negate_value(value));
                break;
            }
            case OP_ADD: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, add_values(a, b));
                break;
            }
            case OP_SUBTRACT: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, subtract_values(a, b));
                break;
            }
            case OP_MULTIPLY: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, multiply_values(a, b));
                break;
            }
            case OP_DIVIDE: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, divide_values(a, b));
                break;
            }
            case OP_MOD: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, mod_values(a, b));
                break;
            }
            case OP_LESS: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, less_values(a, b));
                break;
            }
            case OP_GREATER: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, greater_values(a, b));
                break;
            }
            case OP_GET_VAR: {
                uint8_t slot = read_byte(vm, chunk);
                push(vm, vm->stack[slot]);
                break;
            }
            case OP_SET_VAR: {
                uint8_t slot = read_byte(vm, chunk);
                vm->stack[slot] = peek(vm, 0);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t distance = read_short(vm, chunk);
                if (!(peek(vm, 0).as.boolean_type)) {
                    vm->ip += distance;
                }
                break;
            } 
            case OP_JUMP_IF_TRUE: {
                uint16_t distance = read_short(vm, chunk);
                if ((peek(vm, 0).as.boolean_type)) {
                    vm->ip += distance;
                }
                break;
            } 
            case OP_JUMP: {
                uint16_t distance = read_short(vm, chunk);
                vm->ip += distance;
                break;
            } 
            case OP_JUMP_BACK: {
                uint16_t distance = read_short(vm, chunk);
                vm->ip -= distance;
                break;
            }
            case OP_TRUE: {
                push(vm, to_boolean(true));
                break;
            }
            case OP_FALSE: {
                push(vm, to_boolean(false));
                break;
            }
            case OP_POP: {
                pop(vm);
                break;
            }
            case OP_PRINT: {
                print_value(pop(vm));
                printf("\n");
                break;
            }
            case OP_RETURN: {
                break;
            }
        } 
#ifdef DEBUG_TRACE
        print_trace(vm, op);
#endif
   } 


    return RESULT_SUCCESS;
}

ResultCode compile_and_run(VM* vm, DeclList* dl) {
    Compiler root_compiler;
    init_compiler(&root_compiler);

    ResultCode compile_result = compile(&root_compiler, dl); //TODO: <-- compile is crashing

    if (compile_result == RESULT_FAILED) {
        free_compiler(&root_compiler);
        return RESULT_FAILED; 
    }

    /*
#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(&chunk);
#endif*/
    
    run(vm, &root_compiler.chunk); 

    free_compiler(&root_compiler);

    return RESULT_SUCCESS;
}
