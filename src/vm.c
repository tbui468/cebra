#include "vm.h"
#include "common.h"
#include "memory.h"
#include "obj_string.h"
#include "obj_function.h"
#include "obj_class.h"


#define READ_TYPE(frame, type) \
    (frame->ip += (int)sizeof(type), (type)frame->function->chunk.codes[frame->ip - (int)sizeof(type)])


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

ResultCode init_vm(VM* vm) {
    vm->stack_top = 0;
    vm->frame_count = 0;
    return RESULT_SUCCESS;
}

ResultCode free_vm(VM* vm) {
    return RESULT_SUCCESS;
}

void call(VM* vm, struct ObjFunction* function) {
    CallFrame frame;
    frame.function = function;
    frame.stack_offset = vm->stack_top - function->arity - 1;
    frame.ip = 0;
    frame.arity = function->arity;

    vm->frames[vm->frame_count] = frame;
    vm->frame_count++;
}

static Value read_constant(CallFrame* frame, int idx) {
    return frame->function->chunk.constants.values[idx];
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

ResultCode execute_frame(VM* vm, CallFrame* frame) {
    uint8_t op = READ_TYPE(frame, uint8_t);
    switch(op) {
        case OP_INT: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint8_t)));
            break;
        }
        case OP_FLOAT: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint8_t)));
            break;
        }
        case OP_STRING: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint8_t)));
            break;
        }
        case OP_NIL: {
            push(vm, to_nil());
            break;
        }
        case OP_FUN: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint8_t)));
            break;
        }/*
        case OP_CLASS: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint8_t)));
            break;
        }*/
        case OP_INSTANCE: {
                              /*
            uint8_t class_idx = READ_TYPE(frame, uint8_t) + frame->stack_offset;
            Value klass = vm->stack[class_idx];

            //Table table = ????? //TODO: how to go from klass to this?

            ObjInstance* inst = make_instance(table);
            push(vm, to_instance(inst));*/
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
        case OP_EQUAL: {
            Value b = pop(vm);
            Value a = pop(vm);
            push(vm, equal_values(a, b));
            break;
        }
        case OP_GET_VAR: {
            //uint8_t slot = READ_TYPE(frame, uint8_t) + frame->stack_offset;
            uint8_t slot = READ_TYPE(frame, uint8_t);
            push(vm, vm->stack[slot]);
            break;
        }
        case OP_SET_VAR: {
            //uint8_t slot = READ_TYPE(frame, uint8_t) + frame->stack_offset;
            uint8_t slot = READ_TYPE(frame, uint8_t);
            vm->stack[slot] = peek(vm, 0);
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t distance = READ_TYPE(frame, uint16_t);
            if (!(peek(vm, 0).as.boolean_type)) {
                frame->ip += distance;
            }
            break;
        } 
        case OP_JUMP_IF_TRUE: {
            uint16_t distance = READ_TYPE(frame, uint16_t);
            if ((peek(vm, 0).as.boolean_type)) {
                frame->ip += distance;
            }
            break;
        } 
        case OP_JUMP: {
            uint16_t distance = READ_TYPE(frame, uint16_t);
            frame->ip += distance;
            break;
        } 
        case OP_JUMP_BACK: {
            uint16_t distance = READ_TYPE(frame, uint16_t);
            frame->ip -= distance;
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
        case OP_CALL: {
            int arity = (int)READ_TYPE(frame, uint8_t);
            int start = vm->stack_top - arity - 1;
            struct ObjFunction* function = vm->stack[start].as.function_type;
            call(vm, function);
            break;
        }
        case OP_RETURN: {
            Value ret = ret = pop(vm);
            while (vm->stack_top > frame->stack_offset) {
                pop(vm);
            }
            vm->frame_count--;
            if (ret.type != VAL_NIL) push(vm, ret);
            break;
        }
    } 
#ifdef DEBUG_TRACE
    print_trace(vm, op);
#endif

    return RESULT_SUCCESS;
}

ResultCode compile_and_run(VM* vm, NodeList* nl) {
    struct Compiler root_compiler;
    Chunk chunk;
    init_chunk(&chunk);
    init_compiler(&root_compiler, &chunk);

    ResultCode compile_result = compile_ast(&root_compiler, nl);

    if (compile_result == RESULT_FAILED) {
        free_chunk(&chunk);
        return RESULT_FAILED; 
    }

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(&chunk);
#endif

    struct ObjFunction* script = make_function(chunk, 0);
    push(vm, to_function(script));
    call(vm, script);

    while (vm->frame_count > 0) {
        CallFrame* frame = &vm->frames[vm->frame_count - 1];
        execute_frame(vm, frame);
    }

    //TODO: testing mm.objects - loooks good to me.  Free them here for now until GC works
    free_objects();
    struct Obj* obj = mm.objects;
    int count = 0;
    while (obj != NULL) {
        count++;
        obj = obj->next;
    }
    printf("Total Objects: %d\n", count);

    return RESULT_SUCCESS;
}
