#include "vm.h"
#include "common.h"
#include "memory.h"
#include "obj.h"


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

void pop_stack(VM* vm) {
    while (vm->stack_top > vm->stack) {
        pop(vm);
    }            
}

static Value peek(VM* vm, int depth) {
    return *(vm->stack_top - 1 - depth);
}

ResultCode init_vm(VM* vm) {
    vm->stack_top = &vm->stack[0];
    vm->frame_count = 0;
    vm->open_upvalues = NULL;
    vm->error_count = 0;
    init_table(&vm->globals);

    return RESULT_SUCCESS;
}

ResultCode free_vm(VM* vm) {
    pop_stack(vm);
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

void print_stack(VM* vm) {
    Value* start = vm->stack;
    while (start < vm->stack_top) {
        printf("[ ");
        print_value(*start);
        printf(" ]");
        start++;
    }
}

static void print_trace(VM* vm, OpCode op) {
    //print opcodes - how can the compiler and this use the same code?
    printf("Op: %s\n", op_to_string(op));
    print_stack(vm);
    printf("\n*************************\n");
}

static struct ObjUpvalue* capture_upvalue(VM* vm, Value* location) {

    struct ObjUpvalue* prev = NULL;
    struct ObjUpvalue* curr = vm->open_upvalues;

    if (curr == NULL) {
        vm->open_upvalues = make_upvalue(location);
        return vm->open_upvalues;
    }

    while (curr != NULL && curr->location > location) {
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL) {
        prev->next = make_upvalue(location);
        return prev->next;
    } else if (curr->location == location) {
        return curr;
    } else if (prev == NULL) {
        struct ObjUpvalue* ret = make_upvalue(location);
        ret->next = vm->open_upvalues;
        vm->open_upvalues = ret;
        return ret;
    } else {
        prev->next = make_upvalue(location);
        prev->next->next = curr;
        return prev->next;
    }

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
                    func->upvalues[func->upvalue_count] = capture_upvalue(vm, location);
                    func->upvalue_count++;
                } else {
                    func->upvalues[func->upvalue_count] = frame->function->upvalues[idx];
                    func->upvalue_count++;
                }
            }
            break;
        }
        case OP_CLASS: {
            //[super | nil ]
            Value super_val = pop(vm);
            struct ObjString* struct_string = read_constant(frame, READ_TYPE(frame, uint16_t)).as.string_type;
            struct ObjClass* struct_obj = make_class(struct_string);
            push(vm, to_class(struct_obj));
            //copy all inherited fields
            if (super_val.type != VAL_NIL) {
                struct ObjClass* super = super_val.as.class_type;
                struct_obj->super = super;
                for (int i = 0; i < super->props.capacity; i++) {
                    struct Pair* pair = &super->props.pairs[i];
                    if (pair->key != NULL) {
                        set_table(&struct_obj->props, pair->key, pair->value);
                    }
                } 
            }
            //OP_ADD_PROP is emitted by compiler to add/overwrite properties in klass_val 
            //immediately after OP_CLASS + constant idx
            break;
        }
        case OP_ENUM: {
            struct ObjString* enum_string = read_constant(frame, READ_TYPE(frame, uint16_t)).as.string_type;
            struct ObjEnum* enum_obj = make_enum(enum_string);
            push(vm, to_enum(enum_obj));
            break;
        }
        case OP_ADD_PROP: {
            //current stack: [script]...[struct | enum][value]
            struct ObjString* prop = read_constant(frame, READ_TYPE(frame, uint16_t)).as.string_type;
            Value runtime_obj = peek(vm, 1);
            if (runtime_obj.type == VAL_CLASS) {
                struct ObjClass* klass = runtime_obj.as.class_type;
                set_table(&klass->props, prop, peek(vm, 0));
            } else if (runtime_obj.type == VAL_ENUM) {
                struct ObjEnum* klass = runtime_obj.as.enum_type;
                set_table(&klass->props, prop, peek(vm, 0));
                pop(vm);
            }
            break;
        }
        case OP_INSTANCE: {
            struct ObjClass* klass = pop(vm).as.class_type;
            struct Table props;
            init_table(&props);
            struct ObjInstance* inst = make_instance(props, klass);
            push(vm, to_instance(inst));
            copy_table(&inst->props, &klass->props);
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
            if (peek(vm, 0).type == VAL_NIL) {
                READ_TYPE(frame, uint16_t);
                add_error(vm, "Attempting to access property of a 'nil'.");
                return RESULT_FAILED;
            }

            if (peek(vm, 0).type == VAL_INSTANCE) {
                struct ObjInstance* inst = peek(vm, 0).as.instance_type;
                struct ObjString* prop_name = read_constant(frame, READ_TYPE(frame, uint16_t)).as.string_type;
                Value prop_val = to_nil();
                get_from_table(&inst->props, prop_name, &prop_val);
                pop(vm);
                push(vm, prop_val);
                break;
            }

            if (peek(vm, 0).type == VAL_ENUM) {
                struct ObjEnum* inst = peek(vm, 0).as.enum_type;
                struct ObjString* prop_name = read_constant(frame, READ_TYPE(frame, uint16_t)).as.string_type;
                Value prop_val = to_nil();
                get_from_table(&inst->props, prop_name, &prop_val);
                pop(vm);
                push(vm, prop_val);
                break;
            }
        }
        case OP_SET_PROP: {
            if (peek(vm, 0).type == VAL_NIL) {
                READ_TYPE(frame, uint16_t);
                add_error(vm, "Attempting to set property of a 'nil'.");
                pop(vm);
                return RESULT_FAILED;
            }
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
            struct ObjList* list = make_list();
            push(vm, to_list(list));
            break;
        }
        case OP_MAP: {
            struct ObjMap* map = make_map();
            push(vm, to_map(map));
            break;
        }
        case OP_GET_SIZE: {
            Value value = pop(vm);
            if (value.type == VAL_LIST) {
                struct ObjList* list = value.as.list_type;
                push(vm, to_integer(list->values.count));
                break;
            }
            if (value.type == VAL_STRING) {
                struct ObjString* str = value.as.string_type;
                push(vm, to_integer(str->length));
                break;
            }
        }
        case OP_SET_SIZE: {
            //[new count][list | string]
            Value value = peek(vm, 0);
            int new_count = peek(vm, 1).as.integer_type;
            if (value.type == VAL_LIST) {
                struct ObjList* list = value.as.list_type;
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

            if (value.type == VAL_STRING) {
                struct ObjString* str = value.as.string_type;
                str->chars = GROW_ARRAY(str->chars, char, new_count + 1, str->length + 1);
                if (str->length > new_count) {
                    str->chars[new_count] = '\0';
                } else if (str->length < new_count) {
                    memset(str->chars + str->length, ' ', new_count - str->length);
                    str->chars[new_count] = '\0';
                }
                str->length = new_count;

                pop(vm);
                break;
            }
        }
        case OP_GET_ELEMENT: {
            //[list | map | string][idx]
            Value left = peek(vm, 1);
            if (left.type == VAL_STRING) {
                int idx = pop(vm).as.integer_type;
                struct ObjString* str = left.as.string_type;
                if (idx >= str->length) {
                    add_error(vm, "Index out of bounds.");
                    return RESULT_FAILED;
                }
                pop(vm);
                struct ObjString* c = make_string(str->chars + idx, 1);
                push(vm, to_string(c));
                break; 
            }
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
        case OP_SET_ELEMENT: {
            //[value][list | map | string][idx]
            Value left = peek(vm, 1);
            if (left.type == VAL_STRING) {
                struct ObjString* str = left.as.string_type;
                int idx = peek(vm, 0).as.integer_type;
                Value value = peek(vm, 2); 
                if (value.as.string_type->length > 1) {
                    add_error(vm, "Character at index can only be set to single character string.");
                }

                str->chars[idx] = *(value.as.string_type->chars);

                pop(vm);
                pop(vm);
                break;
            }
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
        case OP_CAST: {
            uint16_t to_type = READ_TYPE(frame, uint16_t);
            Value value = peek(vm, 0);
            if (value.type == VAL_INSTANCE) {
                struct ObjInstance* inst = value.as.instance_type;
                struct ObjString* type = read_constant(frame, to_type).as.string_type;
                Value val;
                if (!get_from_table(&vm->globals, type, &val)) {
                    add_error(vm, "Attempting to cast to undeclared type.");
                    return RESULT_FAILED;
                }
                if (val.type != VAL_CLASS) {
                    add_error(vm, "Attempting to cast to non-struct type.");
                    return RESULT_FAILED;
                }
                struct ObjClass* sub = inst->klass; //This is the actual instance
                struct ObjClass* to = val.as.class_type;

                //check cast down - check to see if target type cast is at or 
                //higher than actual runtime instance type
                bool cast_down = false;
                struct ObjClass* current = sub;
                while (current != NULL) {
                    if (same_string(to->name, current->name)) {
                        cast_down = true;
                        break;
                    }
                    current = current->super;
                }
                if (cast_down) break;

                pop(vm);
                push(vm, to_nil());
                break;
            } else {
                Value result = cast_primitive(to_type, &value);
                pop(vm);
                push(vm, result);
                break;
            }
        }
        case OP_ADD_GLOBAL: {
            Value val = peek(vm, 0);
            struct ObjString* name;
            switch(val.type) {
                case VAL_ENUM:
                    name = val.as.enum_type->name;
                    break;
                case VAL_CLASS:
                    name = val.as.class_type->name;
                    break;
                case VAL_FUNCTION:
                    name = val.as.function_type->name;
                    break;
                case VAL_NATIVE:
                    name = val.as.native_type->name;
                    break;
                default:
                    add_error(vm, "Invalid type - cannot add global.");
                    return RESULT_FAILED;
            }
            set_table(&vm->globals, name, val);
            pop(vm);
            break;
        }
        case OP_GET_GLOBAL: {
            Value name = read_constant(frame, READ_TYPE(frame, uint16_t));
            Value val;
            if (!get_from_table(&vm->globals, name.as.string_type, &val)) {
                add_error(vm, "Global variable not found.");
                return RESULT_FAILED;
            }
            push(vm, val);
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
    }

    if (vm->error_count > 0) {
        for (int i = 0; i < vm->error_count; i++) {
            printf("Runtime Error: ");
            printf("%s\n", vm->errors[i].message);
        }
        pop_stack(vm);

        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}
