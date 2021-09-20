#include <stdio.h>
#include <string.h>

#include "value.h"
#include "memory.h"
#include "obj_string.h"
#include "obj_function.h"
#include "obj_class.h"
#include "obj_instance.h"


Value to_float(double num) {
    Value value;
    value.type = VAL_FLOAT;
    value.as.float_type = num;
    return value;
}

Value to_integer(int32_t num) {
    Value value;
    value.type = VAL_INT;
    value.as.integer_type = num;
    return value;
}

Value to_string(struct ObjString* obj) {
    Value value;
    value.type = VAL_STRING;
    value.as.string_type = obj;
    return value;
}

Value to_boolean(bool b) {
    Value value;
    value.type = VAL_BOOL;
    value.as.boolean_type = b;
    return value;
}

Value to_function(struct ObjFunction* obj) {
    Value value;
    value.type = VAL_FUNCTION;
    value.as.function_type = obj;
    return value;
}

Value to_class(struct ObjClass* obj) {
    Value value;
    value.type = VAL_CLASS;
    value.as.class_type = obj;
    return value;
}

Value to_instance(struct ObjInstance* obj) {
    Value value;
    value.type = VAL_INSTANCE;
    value.as.instance_type = obj;
    return value;
}

Value to_list(struct ObjList* obj) {
    Value value;
    value.type = VAL_LIST;
    value.as.list_type = obj;
    return value;
}

Value to_nil() {
    Value value;
    value.type = VAL_NIL;
    return value;
}

Value to_sig(struct Sig* sig) {
    Value value;
    value.type = VAL_SIG;
    value.as.sig_type = sig;
    return value;
}

Value to_native(struct ObjNative* obj) {
    Value value;
    value.type = VAL_NATIVE;
    value.as.native_type = obj;
    return value;
}

Value negate_value(Value value) {
    if (IS_INT(value)) {
        return to_integer(-value.as.integer_type);
    } else if (IS_FLOAT(value)) {
        return to_float(-value.as.float_type);
    } else if (IS_BOOL(value)) {
        return to_boolean(!value.as.boolean_type);
    }
}

Value add_values(Value a, Value b) {
    if (IS_INT(b)) {
        return to_integer(a.as.integer_type + b.as.integer_type);
    } else if (IS_FLOAT(b)) {
        return to_float(a.as.float_type + b.as.float_type);
    } else if (IS_STRING(b)) {
        struct ObjString* left = a.as.string_type;
        struct ObjString* right = b.as.string_type;

        int length =  left->length + right->length;
        char* concat = ALLOCATE_ARRAY(char);
        concat = GROW_ARRAY(concat, char, length + 1, 0);
        memcpy(concat, left->chars, left->length);
        memcpy(concat + left->length, right->chars, right->length);
        concat[length] = '\0';

        struct ObjString* obj = take_string(concat, length);
        return to_string(obj);
    }
}
Value subtract_values(Value a, Value b) {
    if (IS_INT(b)) {
        return to_integer(a.as.integer_type - b.as.integer_type);
    } else {
        return to_float(a.as.float_type - b.as.float_type);
    }
}
Value multiply_values(Value a, Value b) {
    if (IS_INT(b)) {
        return to_integer(a.as.integer_type * b.as.integer_type);
    } else {
        return to_float(a.as.float_type * b.as.float_type);
    }
}
Value divide_values(Value a, Value b) {
    if (IS_INT(b)) {
        return to_integer(a.as.integer_type / b.as.integer_type);
    } else {
        return to_float(a.as.float_type / b.as.float_type);
    }
}

Value less_values(Value a, Value b) {
    if (IS_INT(b)) {
        return to_boolean(a.as.integer_type < b.as.integer_type);
    } else {
        return to_boolean(a.as.float_type < b.as.float_type);
    }
}

Value greater_values(Value a, Value b) {
    if (IS_INT(b)) {
        return to_boolean(a.as.integer_type > b.as.integer_type);
    } else {
        return to_boolean(a.as.float_type > b.as.float_type);
    }
}

Value mod_values(Value a, Value b) {
    return to_integer(a.as.integer_type % b.as.integer_type);
}


Value equal_values(Value a, Value b) {
    if (a.type != b.type) return to_boolean(false);

    switch(b.type) {
        case VAL_INT:
            return to_boolean(a.as.integer_type == b.as.integer_type);
        case VAL_FLOAT:
            return to_boolean(a.as.float_type == b.as.float_type);
        case VAL_STRING:
            struct ObjString* s1 = a.as.string_type;
            struct ObjString* s2 = b.as.string_type;
            if (s1->length != s2->length) return to_boolean(false);
            return to_boolean(memcmp(s1->chars, s2->chars, s1->length) == 0);
    }
}

void print_value(Value a) {
    switch(a.type) {
        case VAL_INT:
            printf("%d", a.as.integer_type);
            break;
        case VAL_FLOAT:
            printf("%f", a.as.float_type);
            break;
        case VAL_BOOL:
            printf("%s", a.as.boolean_type ? "true" : "false");
            break;
        case VAL_STRING:
            printf("%s", a.as.string_type->chars);
            break;
        case VAL_FUNCTION:
            printf("%s", "<fun: ");
            printf("%s", a.as.function_type->name->chars);
            printf(">");
            break;
        case VAL_CLASS:
            printf("%s", "<class>");
            break;
        case VAL_INSTANCE:
            printf("%s", "<instance>");
            break;
        case VAL_NIL:
            printf("%s", "<Nil>");
            break;
        case VAL_SIG:
            printf("%s", "<sig>");
            break;
        case VAL_NATIVE:
            printf("%s", "<native>");
            break;
        case VAL_LIST:
            printf("%s", "<list>");
            break;
        default:
            printf("Invalid value");
            break;
    }
}

const char* value_type_to_string(ValueType type) {
    switch(type) {
        case VAL_INT: return "VAL_INT";
        case VAL_FLOAT: return "VAL_FLOAT";
        case VAL_BOOL: return "VAL_BOOL";
        case VAL_STRING: return "VAL_STRING";
        case VAL_FUNCTION: return "VAL_FUNCTION";
        case VAL_CLASS: return "VAL_CLASS";
        case VAL_INSTANCE: return "VAL_INSTANCE";
        case VAL_NIL: return "VAL_NIL";
        case VAL_SIG: return "VAL_SIG";
        case VAL_NATIVE: return "VAL_NATIVE";
        case VAL_LIST: return "VAL_LIST";
        default: return "Unrecognized VAL_TYPE";
    }
}

struct Obj* get_object(Value* value) {
    switch (value->type) {
        case VAL_STRING: {
            struct ObjString* obj = value->as.string_type;
            return (struct Obj*)obj;
        }
        case VAL_FUNCTION: {
            struct ObjFunction* obj = value->as.function_type;
            return (struct Obj*)obj;
        }
        case VAL_CLASS: {
            struct ObjClass* obj = value->as.class_type;
            return (struct Obj*)obj;
        }
        case VAL_INSTANCE: {
            struct ObjInstance* obj = value->as.instance_type;
            return (struct Obj*)obj;
        }
        case VAL_NATIVE: {
            struct ObjNative* obj = value->as.native_type;
            return (struct Obj*)obj;
        }
        case VAL_LIST: {
            struct ObjList* obj = value->as.list_type;
            return (struct Obj*)obj;
        }
        //Values with stack allocated data
        //don't need to be garbage collected
        case VAL_INT:
        case VAL_FLOAT:
        case VAL_BOOL:
        case VAL_NIL:
        case VAL_SIG:
            return NULL;
    }
}


Value copy_value(Value* value) {
    switch (value->type) {
        case VAL_INT:
            return to_integer(value->as.integer_type);
        case VAL_FLOAT:
            return to_float(value->as.float_type);
        case VAL_BOOL:
            return to_boolean(value->as.boolean_type);
            break;
        case VAL_STRING:
            struct ObjString* str = value->as.string_type;
            struct ObjString* copy = make_string(str->chars, str->length);
            return to_string(copy);
        default:
            break;
    }
}
