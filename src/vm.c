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
    init_table(&frame.defs);

    vm->frames[vm->frame_count] = frame;
    vm->frame_count++;
}

void free_frame(CallFrame* frame) {
    free_table(&frame->defs);
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

static Value find_def(VM* vm, struct ObjString* str) {
    for (int i = vm->frame_count - 1; i >= 0; i--) {
        CallFrame* frame = &vm->frames[i];
        Value value;
        if (get_from_table(&frame->defs, str, &value)) {
            return value;
        }
    }

    //TODO: add runtim error if not found, but this should be caught in compiler
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
        /*
        case OP_FUN: {
            Value fun;
            get_value(&frame->function->chunk->defs, 
            set_key_value(&frame->function->defs,
                          str,
                          read_constant(frame, READ_TYPE(frame, uint8_t)))
            break;
        }
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
            uint8_t slot = READ_TYPE(frame, uint8_t) + frame->stack_offset;
            push(vm, vm->stack[slot]);
            break;
        }
        case OP_SET_VAR: {
            uint8_t slot = READ_TYPE(frame, uint8_t) + frame->stack_offset;
            vm->stack[slot] = peek(vm, 0);
            break;
        }
        case OP_SET_DEF: {
            Value str_value = read_constant(frame, READ_TYPE(frame, uint8_t));
            Value fun_value = read_constant(frame, READ_TYPE(frame, uint8_t));
            set_table(&frame->defs, str_value.as.string_type, fun_value);
            break;
        }
        case OP_GET_DEF: {
            Value str_value = read_constant(frame, READ_TYPE(frame, uint8_t));
            Value fun_def = find_def(vm, str_value.as.string_type);
            push(vm, fun_def);
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
            //TODO: this is ugly - rewrite this
            //we want to cache the top of the callframe stack (the return value)
            //before popping everything, and only push it if it's NOT nil
            Value ret = to_nil();
            if (vm->stack_top > frame->stack_offset + 1 + frame->arity) {
                ret = pop(vm);
            }
            while (vm->stack_top > frame->stack_offset) {
                pop(vm);
            }
            free_frame(frame);
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
    Compiler root_compiler;
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
