#include "vm.h"


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

static Value get_constant(Chunk* chunk, int idx) {
    return chunk->constants[idx];
}

ResultCode vm_init(VM* vm) {
    vm->stack_top = 0;
    vm->ip = 0;
    return RESULT_SUCCESS;
}

ResultCode vm_free(VM* vm) {
    return RESULT_SUCCESS;
}

ResultCode execute(VM* vm, Chunk* chunk) {
   vm_init(vm); //TODO: need to think of better way to reset ip (but not necessarily the stack - for REPL)
   while (vm->ip < chunk->count) {
        switch(read_byte(vm, chunk)) {
            case OP_INT: {
                uint8_t idx = read_byte(vm, chunk);
                push(vm, get_constant(chunk, idx));
                break;
            }
            case OP_FLOAT: {
                uint8_t idx = read_byte(vm, chunk);
                push(vm, get_constant(chunk, idx));
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
            case OP_PRINT: {
                Value value = pop(vm);
                if (value.type == VAL_INT) {
                    printf("%d\n", value.as.integer_type);
                } else {
                    printf("%f\n", value.as.float_type);
                }
                break;
            }
            case OP_RETURN: {
                break;
            }
        } 
   } 

   return RESULT_SUCCESS;
}
