#include "vm.h"
#include "common.h"
#include "memory.h"
#include "obj_string.h"
#include "obj_function.h"
#include "obj_class.h"
#include "obj_instance.h"


#define READ_TYPE(frame, type) \
    (frame->ip += (int)sizeof(type), (type)frame->function->chunk.codes[frame->ip - (int)sizeof(type)])

static void add_error(VM* vm, const char* message) {
    RuntimeError error;
    error.message = message;
    vm->errors[vm->error_count] = error;
    vm->error_count++;
}

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
    vm->error_count = 0;

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

    struct ObjUpvalue* previous = NULL;
    struct ObjUpvalue* current = vm->open_upvalues;
    struct ObjUpvalue* next = current->next;
    while (next != NULL && location > current->location) {
        previous = current;
        current = next;
        next = next->next;
    }

    if (location == current->location) {
        return current;
    }

    struct ObjUpvalue* new_upvalue = make_upvalue(location);
    if (location > current->location) {
        new_upvalue->next = current;
        if (previous == NULL) {
            vm->open_upvalues = new_upvalue;
        } else {
            previous->next = new_upvalue;
        }
    } else {
        current->next = new_upvalue;
        new_upvalue->next = next;
    }
    return new_upvalue;
}

static void close_upvalues(VM* vm, Value* location) {
    while (vm->open_upvalues != NULL && vm->open_upvalues->location >= location) {
        vm->open_upvalues->closed = *(vm->open_upvalues->location);
        vm->open_upvalues->location = &vm->open_upvalues->closed; 
        vm->open_upvalues = vm->open_upvalues->next;
    }
}

ResultCode execute_frame(VM* vm, CallFrame* frame) {
    uint8_t op = READ_TYPE(frame, uint8_t);
    switch(op) {
        case OP_CONSTANT: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint16_t)));
            break;
        }
        case OP_NIL: {
            push(vm, to_nil());
            break;
        }
        case OP_FUN: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint16_t)));
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
        }
        case OP_CLASS: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint16_t)));
            break;
        }
        case OP_ADD_PROP: {
            //This op only gets added after a class property/method is compiled into a local variable
            //current stack: [script]...[class][value]
            push_root(read_constant(frame, READ_TYPE(frame, uint16_t)));
            struct ObjClass* klass = peek(vm, 2).as.class_type;
            set_table(&klass->props, peek(vm, 0).as.string_type, peek(vm, 1));
            pop_root();
            pop(vm);
            break;
        }
        case OP_INSTANCE: {
            struct ObjClass* klass = pop(vm).as.class_type;
            struct Table props = copy_table(&klass->props);
            struct ObjInstance* inst = make_instance(props);
            push(vm, to_instance(inst));
            break;
        }
        case OP_NEGATE: {
            Value value = pop(vm);
            push(vm, negate_value(value));
            break;
        }
        case OP_ADD: {
            Value b = peek(vm, 0);
            Value a = peek(vm, 1);
            Value result = add_values(a, b);
            pop(vm);
            pop(vm);
            push(vm, result);
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
        case OP_GET_PROP: {
            struct ObjInstance* inst = peek(vm, 0).as.instance_type;
            struct ObjString* prop_name = read_constant(frame, READ_TYPE(frame, uint16_t)).as.string_type;
            Value prop_val = to_nil();
            get_from_table(&inst->props, prop_name, &prop_val);
            pop(vm);
            push(vm, prop_val);
            break;
        }
        case OP_SET_PROP: {
            struct ObjInstance* inst = peek(vm, 0).as.instance_type;
            Value value = peek(vm, 1);
            struct ObjString* prop_name = read_constant(frame, READ_TYPE(frame, uint16_t)).as.string_type;
            set_table(&inst->props, prop_name, value);
            pop(vm);
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
        case OP_CALL: {
            int arity = (int)READ_TYPE(frame, uint8_t);
            Value value = peek(vm, arity);
            if (value.type == VAL_FUNCTION) {
                call(vm, value.as.function_type);
            }
            if (value.type == VAL_NATIVE) {
                Value (*native)(int, Value*) = value.as.native_type->function;
                Value result = native(arity, vm->stack_top - arity);
                for (int i = 0; i < arity + 1; i++) {
                    pop(vm);
                }
                if (result.type != VAL_NIL) {
                    push(vm, result);
                }
            }
            break;
        }
        case OP_RETURN: {
            Value ret = pop(vm);
            close_upvalues(vm, frame->locals); 
            vm->stack_top = frame->locals;
            vm->frame_count--;
            if (ret.type != VAL_NIL) push(vm, ret);
            break;
        }
        case OP_NATIVE: {
            push(vm, read_constant(frame, READ_TYPE(frame, uint16_t)));
            break;
        }
        case OP_LIST: {
            struct ObjList* list = make_list(peek(vm, 0));
            pop(vm);
            push(vm, to_list(list));
            break;
        }
        case OP_MAP: {
            struct ObjMap* map = make_map(peek(vm, 0));
            pop(vm);
            push(vm, to_map(map));
            break;
        }
        case OP_GET_SIZE: {
            struct ObjList* list = pop(vm).as.list_type;
            push(vm, to_integer(list->values.count));
            break;
        }
        case OP_SET_SIZE: {
            //[new count][list]
            struct ObjList* list = peek(vm, 0).as.list_type;
            int new_count = peek(vm, 1).as.integer_type;
            if (new_count > list->values.capacity) {
                int new_cap = list->values.capacity == 0 ? 8 : list->values.capacity * 2;
                while (new_cap < new_count) {
                    new_cap *= 2;
                }
                list->values.values = GROW_ARRAY(list->values.values, Value, new_cap, list->values.capacity);
                list->values.capacity = new_cap;
            }
            while (list->values.count < new_count) {
                list->values.values[list->values.count] = list->default_value;
                list->values.count++;
            }
            list->values.count = new_count;
            pop(vm);
            break;
        }
        case OP_GET_IDX: {
            //[list | map][idx]
            Value left = peek(vm, 1);
            if (left.type == VAL_LIST) {
                int idx = pop(vm).as.integer_type;
                struct ObjList* list = left.as.list_type;
                if (idx >= list->values.count) {
                    add_error(vm, "Index out of bounds.");
                    return RESULT_FAILED;
                }
                pop(vm);
                push(vm, list->values.values[idx]);
                break;
            }
            if (left.type == VAL_MAP) {
                struct ObjString* key = pop(vm).as.string_type;
                struct ObjMap* map = left.as.map_type;
                Value value = to_nil();
                get_from_table(&map->table, key, &value);
                if (value.type == VAL_NIL) {
                    value = map->default_value;
                }
                pop(vm);
                push(vm, value);
                break;
            }
        }
        case OP_SET_IDX: {
            //[value][list | map][idx]
            Value left = peek(vm, 1);
            if (left.type == VAL_LIST) {
                struct ObjList* list = left.as.list_type;
                int idx = peek(vm, 0).as.integer_type;
                Value value = peek(vm, 2);
                if (idx >= list->values.count) {
                    while (list->values.count < idx) {
                        add_value(&list->values, list->default_value);
                    }
                    add_value(&list->values, value);
                } else {
                    list->values.values[idx] = value;
                }
                pop(vm);
                pop(vm);
                break;
            }
            if (left.type == VAL_MAP) {
                struct ObjMap* map = left.as.map_type;
                struct ObjString* key = peek(vm, 0).as.string_type;
                Value value = peek(vm, 2);
                set_table(&map->table, key, value);
                pop(vm);
                pop(vm);
                break;
            }
        }
        case OP_IN_LIST: {
            //[value][list]
            struct ObjList* list = peek(vm, 0).as.list_type;
            Value value = peek(vm, 1);
            bool in_list = false;
            for (int i = 0; i < list->values.count; i++) {
                if (equal_values(value, list->values.values[i]).as.boolean_type) {
                    in_list = true;
                    break;
                }                
            }
            pop(vm);
            pop(vm);
            push(vm, to_boolean(in_list));
            break;
        }
        case OP_GET_KEYS: {
            struct ObjMap* map = pop(vm).as.map_type;
            struct ObjString* default_string = make_string("Invalid key", 11);
            struct ObjList* list = make_list(to_string(default_string));
            push(vm, to_list(list));
            for (int i = 0; i < map->table.capacity; i++) {
                struct Pair* pair = &map->table.pairs[i];
                if (pair->key != NULL) {
                    add_value(&list->values, to_string(pair->key));
                }
            }
            break;
        }
        case OP_GET_VALUES: {
            struct ObjMap* map = pop(vm).as.map_type;
            struct ObjString* default_string = make_string("Invalid key", 11);
            struct ObjList* list = make_list(to_string(default_string));
            push(vm, to_list(list));
            for (int i = 0; i < map->table.capacity; i++) {
                struct Pair* pair = &map->table.pairs[i];
                if (pair->key != NULL) {
                    add_value(&list->values, pair->value);
                }
            }
            break;
        }
    } 

#ifdef DEBUG_TRACE
    print_trace(vm, op);
#endif

    return RESULT_SUCCESS;
}

ResultCode run(VM* vm, struct ObjFunction* script) {

    push(vm, to_function(script));
    call(vm, script);

    while (vm->frame_count > 0) {
        CallFrame* frame = &vm->frames[vm->frame_count - 1];
        ResultCode result = execute_frame(vm, frame);
        if (vm->error_count > 0) {
            for (int i = 0; i < vm->error_count; i++) {
                printf("%s\n", vm->errors[i].message);
            }
            return RESULT_FAILED;
        }
    }

    return RESULT_SUCCESS;
}
