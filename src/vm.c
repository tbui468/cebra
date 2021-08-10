#include "vm.h"
#include "object.h"


static Value pop(VM* vm) {
    vm->stack_top--;
    return vm->stack[vm->stack_top];
}

static void push(VM* vm, Value value) {
    vm->stack[vm->stack_top] = value;
    vm->stack_top++;
}

static uint8_t read_byte(VM* vm, Chunk* chunk) {
    vm->ip++;
    return chunk->codes[vm->ip - 1]; 
}

ResultCode vm_init(VM* vm) {
    vm->stack_top = 0;
    vm->ip = 0;
    return RESULT_SUCCESS;
}

ResultCode vm_free(VM* vm) {
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

ResultCode execute(VM* vm, Chunk* chunk) {
   vm_init(vm); //TODO: need to think of better way to reset ip (but not necessarily the stack - for REPL)
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
            case OP_GET_VAR: {
                uint8_t slot = read_byte(vm, chunk);
                push(vm, vm->stack[slot]);
                break;
            }
            case OP_SET_VAR: {
                uint8_t slot = read_byte(vm, chunk);
                vm->stack[slot] = pop(vm);
                push(vm, vm->stack[slot]);
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
        print_trace(vm, op);
   } 

   return RESULT_SUCCESS;
}
