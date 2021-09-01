#include "vm.h"
#include "common.h"
#include "memory.h"
#include "obj_string.h"
#include "obj_function.h"
#include "obj_class.h"


#define READ_TYPE(frame, type) \
    (frame->ip += (int)sizeof(type), (type)frame->function->chunk.codes[frame->ip - (int)sizeof(type)])


Value pop(VM* vm) {
    vm->stack_top--;
    return *vm->stack_top;
}

void push(VM* vm, Value value) {
    *vm->stack_top = value;
    vm->stack_top++;
}

static Value peek(VM* vm, int depth) {
    return *(vm->stack_top - 1 - depth);
}

ResultCode init_vm(VM* vm) {
    vm->stack_top = &vm->stack[0];
    vm->frame_count = 0;
    vm->open_upvalues = NULL;
    return RESULT_SUCCESS;
}

ResultCode free_vm(VM* vm) {
    return RESULT_SUCCESS;
}

void call(VM* vm, struct ObjFunction* function) {
    CallFrame frame;
    frame.function = function;
    frame.locals = vm->stack_top - function->arity - 1;
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
    Value* start = vm->stack;
    while (start < vm->stack_top) {
        printf("[ ");
        print_value(*start);
        printf(" ]");
        start++;
    }
    printf("\n*************************\n");
}

static struct ObjUpvalue* capture_upvalue(VM* vm, Value* location) {

    if (vm->open_upvalues == NULL) {
        struct ObjUpvalue* new_upvalue = make_upvalue(location);
        vm->open_upvalues = new_upvalue;
        return new_upvalue;
    }

    struct ObjUpvalue* current = vm->open_upvalues;
    struct ObjUpvalue* next = current->next;
    while (next != NULL && location > current->location) {
        current = next;
        next = next->next;
    }

    if (location == current->location) {
        return current;
    }

    struct ObjUpvalue* new_upvalue = make_upvalue(location);
    current->next = new_upvalue;
    new_upvalue->next = next;
    return new_upvalue;
}

static void close_upvalues(VM* vm, Value* location) {
    struct ObjUpvalue* current = vm->open_upvalues;
    while (current != NULL && current->location >= location) {
        current->closed = *location;
        current->location = &current->closed;
        current = current->next;
    }
}

ResultCode execute_frame(VM* vm, CallFrame* frame) {
    uint8_t op = READ_TYPE(frame, uint8_t);
    switch(op) {
        case OP_CONSTANT: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint8_t)));
            break;
        }
        case OP_NIL: {
            push(vm, to_nil());
            break;
        }
        case OP_FUN: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint8_t)));
            struct ObjFunction* func = peek(vm, 0).as.function_type;
            int total_upvalues = READ_TYPE(frame, uint8_t);
            for (int i = 0; i < total_upvalues; i++) {
                bool is_local = READ_TYPE(frame, uint8_t);
                int idx = READ_TYPE(frame, uint8_t);
                if (is_local) {
                    Value* location = &frame->locals[idx];
                    func->upvalues[func->upvalue_count++] = capture_upvalue(vm, location);
                } else {
                    func->upvalues[func->upvalue_count++] = frame->function->upvalues[idx];
                }
            }
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
        case OP_GET_LOCAL: {
            uint8_t slot = READ_TYPE(frame, uint8_t);
            push(vm, frame->locals[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            uint8_t slot = READ_TYPE(frame, uint8_t);
            frame->locals[slot] = peek(vm, 0);
            break;
        }
        case OP_GET_UPVALUE: {
            uint8_t slot = READ_TYPE(frame, uint8_t);
            push(vm, *(frame->function->upvalues[slot]->location));
            break;
        }
        case OP_SET_UPVALUE: {
            uint8_t slot = READ_TYPE(frame, uint8_t);
            *frame->function->upvalues[slot]->location = peek(vm, 0);
            break;
        }
        case OP_CLOSE_UPVALUE: {
            close_upvalues(vm, vm->stack_top - 1);
            pop(vm);
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
            struct ObjFunction* function = peek(vm, arity).as.function_type;
            call(vm, function);
            break;
        }
        case OP_RETURN: {
            Value ret = ret = pop(vm);
            close_upvalues(vm, frame->locals + 1);
            vm->stack_top = frame->locals;
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

    struct ObjFunction* script = make_function(0);
    struct Compiler script_compiler;
    init_compiler(&script_compiler, script);
    cc = &script_compiler;

    ResultCode compile_result = compile_script(&script_compiler, nl);

    if (compile_result == RESULT_FAILED) {
        return RESULT_FAILED; 
    }

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(&chunk);
#endif

    push(vm, to_function(script));
    call(vm, script);

    while (vm->frame_count > 0) {
        CallFrame* frame = &vm->frames[vm->frame_count - 1];
        execute_frame(vm, frame);
    }

    cc = NULL;

    return RESULT_SUCCESS;
}
