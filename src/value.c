#include <stdio.h>
#include <string.h>

#include "value.h"
#include "memory.h"

/*
void init_sig_list(SigList* sl) {
    sl->types = ALLOCATE_ARRAY(ValueType);
    sl->count = 0;
    sl->capacity = 0;
}

void free_sig_list(SigList* sl) {
    FREE_ARRAY(sl->types, ValueType, sl->capacity);
}

void add_sig_type(SigList* sl, ValueType type) {
    if (sl->count + 1 > sl->capacity) {
        int new_capacity = sl->capacity == 0 ? 8 : sl->capacity * 2;
        sl->types = GROW_ARRAY(sl->types, ValueType, new_capacity, sl->capacity);
        sl->capacity = new_capacity;
    }

    sl->types[sl->count] = type;
    sl->count++;
}

SigList copy_sig_list(SigList* sl) {
    SigList copy;
    init_sig_list(&copy);

    for (int i = 0; i < sl->count; i++) {
        add_sig_type(&copy, sl->types[i]);
    }

    return copy;
}*/

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

Value to_string(ObjString* obj) {
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

Value to_function(ObjFunction* obj) {
    Value value;
    value.type = VAL_FUNCTION;
    value.as.function_type = obj;
    return value;
}

Value to_nil() {
    Value value;
    value.type = VAL_NIL;
    return value;
}

Value negate_value(Value value) {
    if (value.type == VAL_INT) {
        return to_integer(-value.as.integer_type);
    } else if (value.type == VAL_FLOAT) {
        return to_float(-value.as.float_type);
    } else if (value.type == VAL_BOOL) {
        return to_boolean(!value.as.boolean_type);
    }
}

Value add_values(Value a, Value b) {
    if (b.type == VAL_INT) {
        return to_integer(a.as.integer_type + b.as.integer_type);
    } else if (b.type == VAL_FLOAT) {
        return to_float(a.as.float_type + b.as.float_type);
    } else if (b.type == VAL_STRING) {
        ObjString* left = a.as.string_type;
        ObjString* right = b.as.string_type;

        int length =  left->length + right->length;
        char* concat = ALLOCATE_CHAR(length + 1);
        memcpy(concat, left->chars, left->length);
        memcpy(concat + left->length, right->chars, right->length);
        concat[length] = '\0';

        ObjString* obj = take_string(concat, length);
        return to_string(obj);
    }
}
Value subtract_values(Value a, Value b) {
    if (b.type == VAL_INT) {
        return to_integer(a.as.integer_type - b.as.integer_type);
    } else {
        return to_float(a.as.float_type - b.as.float_type);
    }
}
Value multiply_values(Value a, Value b) {
    if (b.type == VAL_INT) {
        return to_integer(a.as.integer_type * b.as.integer_type);
    } else {
        return to_float(a.as.float_type * b.as.float_type);
    }
}
Value divide_values(Value a, Value b) {
    if (b.type == VAL_INT) {
        return to_integer(a.as.integer_type / b.as.integer_type);
    } else {
        return to_float(a.as.float_type / b.as.float_type);
    }
}

Value less_values(Value a, Value b) {
    if (b.type == VAL_INT) {
        return to_boolean(a.as.integer_type < b.as.integer_type);
    } else {
        return to_boolean(a.as.float_type < b.as.float_type);
    }
}

Value greater_values(Value a, Value b) {
    if (b.type == VAL_INT) {
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
            ObjString* s1 = a.as.string_type;
            ObjString* s2 = a.as.string_type;
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
        case VAL_STRING:
            printf("%s", a.as.string_type->chars);
            break;
        case VAL_BOOL:
            printf("%s", a.as.boolean_type ? "true" : "false");
            break;
        case VAL_FUNCTION:
            printf("%s", "<fun>");
            break;
        case VAL_NIL:
            printf("%s", "<Nil>");
            break;
    }
}

ValueType get_value_type(Token token) {
    switch(token.type) {
        case TOKEN_INT_TYPE: return VAL_INT;
        case TOKEN_FLOAT_TYPE: return VAL_FLOAT;
        case TOKEN_STRING_TYPE: return VAL_STRING;
        case TOKEN_BOOL_TYPE: return VAL_BOOL;
        case TOKEN_NIL_TYPE: return VAL_NIL;
    }
}
