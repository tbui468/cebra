#include <stdio.h>
#include <string.h>

#include "value.h"
#include "memory.h"
#include "obj.h"


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

Value to_byte(uint8_t byte) {
    Value value;
    value.type = VAL_BYTE;
    value.as.byte_type = byte;
    return value;
}

Value to_function(struct ObjFunction* obj) {
    Value value;
    value.type = VAL_FUNCTION;
    value.as.function_type = obj;
    return value;
}

Value to_struct(struct ObjStruct* obj) {
    Value value;
    value.type = VAL_STRUCT;
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

Value to_map(struct ObjMap* obj) {
    Value value;
    value.type = VAL_MAP;
    value.as.map_type = obj;
    return value;
}

Value to_enum(struct ObjEnum* obj) {
    Value value;
    value.type = VAL_ENUM;
    value.as.enum_type = obj;
    return value;
}

Value to_file(struct ObjFile* obj) {
    Value value;
    value.type = VAL_FILE;
    value.as.file_type = obj;
    return value;
}

Value to_nil() {
    Value value;
    value.type = VAL_NIL;
    return value;
}

Value to_type(struct Type* type) {
    Value value;
    value.type = VAL_TYPE;
    value.as.type_type = type;
    return value;
}

Value to_native(struct ObjNative* obj) {
    Value value;
    value.type = VAL_NATIVE;
    value.as.native_type = obj;
    return value;
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
            return to_boolean(a.as.string_type == b.as.string_type);
        case VAL_BOOL:
            return to_boolean(a.as.boolean_type == b.as.boolean_type);
        case VAL_NIL:
            return to_boolean(true);
        default:
            return to_boolean(false); //TODO: need to fill in other types here
    }
}

static Value cast_to_string(Value* value) {
    switch(value->type) {
        case VAL_INT: {
            int num = value->as.integer_type;

            char* str = ALLOCATE_ARRAY(char);
            str = GROW_ARRAY(str, char, 80, 0);
            int len = sprintf(str, "%d", num);
            str = GROW_ARRAY(str, char, len + 1, 80);

            return to_string(take_string(str, len));
        }
        case VAL_FLOAT: {
            double num = value->as.float_type;

            char* str = ALLOCATE_ARRAY(char);
            str = GROW_ARRAY(str, char, 80, 0);
            int len = sprintf(str, "%f", num);
            str = GROW_ARRAY(str, char, len + 1, 80);

            return to_string(take_string(str, len));
        }
        case VAL_BOOL:
            if (value->as.boolean_type) {
                return to_string(make_string("true", 4));
            }
            return to_string(make_string("false", 5));
        case VAL_NIL:
            return to_string(make_string("nil", 3));
        default:
            return to_nil();
    }
}

static Value cast_to_int(Value* value) {
    switch(value->type) {
        case VAL_STRING: {
            char* end;
            long i = strtol(value->as.string_type->chars, &end, 10);
            return to_integer(i);
        }
        case VAL_BYTE:
            return to_integer((int)(value->as.byte_type));
        case VAL_FLOAT:
            return to_integer((int)(value->as.float_type));
        case VAL_BOOL:
            if (value->as.boolean_type) return to_integer(1);
            return to_integer(0);
        default:
            return to_nil();
    }
}

static Value cast_to_byte(Value* value) {
    switch(value->type) {
        case VAL_STRING: {
            char* end;
            uint8_t i = (uint8_t)strtol(value->as.string_type->chars, &end, 10);
            return to_byte(i);
        }
        case VAL_FLOAT:
            return to_byte((uint8_t)(value->as.float_type));
        case VAL_BOOL:
            if (value->as.boolean_type) return to_integer(1);
            return to_byte(0);
        default:
            return to_nil();
    }
}

static Value cast_to_float(Value* value) {
    switch(value->type) {
        case VAL_STRING: {
            char* end;
            double d = strtod(value->as.string_type->chars, &end);
            return to_float(d);
        }
        case VAL_INT:
            return to_float((double)(value->as.integer_type));
        case VAL_BYTE:
            return to_float((double)(value->as.byte_type));
        case VAL_BOOL:
            if (value->as.boolean_type) return to_float(1.0);
            return to_float(0.0);
        default:
            return to_nil();
    }
}
static Value cast_to_bool(Value* value) {
    switch(value->type) {
        case VAL_STRING: {
            struct ObjString* str = value->as.string_type;
            if (str->length == 0) return to_boolean(false);
            return to_boolean(true);
        }
        case VAL_INT:
            if (value->as.integer_type <= 0) return to_boolean(false);
            return to_boolean(true);
        case VAL_FLOAT:
            if (value->as.float_type <= 0.0) return to_boolean(false);
            return to_boolean(true);
        default:
            return to_nil();
    }
}

Value cast_primitive(ValueType to_type, Value* value) {
    switch(to_type) {
        case VAL_STRING:
            return cast_to_string(value);
        case VAL_INT:
            return cast_to_int(value);
        case VAL_BYTE:
            return cast_to_byte(value);
        case VAL_FLOAT:
            return cast_to_float(value);
        case VAL_BOOL:
            return cast_to_bool(value);
        default:
            return to_nil();
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
        case VAL_BYTE:
            printf("%d", a.as.byte_type);
            break;
        case VAL_STRING:
            printf("<string %s >", a.as.string_type->chars);
            break;
        case VAL_FUNCTION:
            printf("%s", "<fun: ");
            //printf("%s", a.as.function_type->name->chars);
            printf(">");
            break;
        case VAL_STRUCT:
            printf("%s", "<struct: ");
            print_object((struct Obj*)(a.as.class_type->name));
            printf(">");
            break;
        case VAL_INSTANCE:
            printf("%s", "<instance>");
            break;
        case VAL_ENUM:
            printf("%s", "<enum>");
            break;
        case VAL_NIL:
            printf("%s", "<nil>");
            break;
        case VAL_TYPE:
            printf("%s", "<type> ");
            print_type(a.as.type_type);
            break;
        case VAL_NATIVE:
            printf("%s", "<native>");
            break;
        case VAL_LIST:
            printf("%s", "<list>");
            break;
        case VAL_MAP:
            printf("%s", "<map>");
            break;
        case VAL_FILE:
            printf("%s", "<file>");
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
        case VAL_BYTE: return "VAL_BYTE";
        case VAL_STRING: return "VAL_STRING";
        case VAL_FUNCTION: return "VAL_FUNCTION";
        case VAL_STRUCT: return "VAL_STRUCT";
        case VAL_INSTANCE: return "VAL_INSTANCE";
        case VAL_NIL: return "VAL_NIL";
        case VAL_TYPE: return "VAL_TYPE";
        case VAL_NATIVE: return "VAL_NATIVE";
        case VAL_LIST: return "VAL_LIST";
        case VAL_MAP: return "VAL_MAP";
        case VAL_ENUM: return "VAL_ENUM";
        case VAL_FILE: return "VAL_FILE";
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
        case VAL_STRUCT: {
            struct ObjStruct* obj = value->as.class_type;
            return (struct Obj*)obj;
        }
        case VAL_INSTANCE: {
            struct ObjInstance* obj = value->as.instance_type;
            return (struct Obj*)obj;
        }
        case VAL_ENUM: {
            struct ObjEnum* obj = value->as.enum_type;
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
        case VAL_MAP: {
            struct ObjMap* obj = value->as.map_type;
            return (struct Obj*)obj;
        }
        case VAL_FILE: {
            struct ObjFile* obj = value->as.file_type;
            return (struct Obj*)obj;
        }
        //Values with stack allocated data
        //don't need to be garbage collected
        case VAL_INT:
        case VAL_FLOAT:
        case VAL_BOOL:
        case VAL_BYTE:
        case VAL_NIL:
        case VAL_TYPE:
            return NULL;
    }
}

Value copy_value(Value* value) {
    switch (value->type) {
        case VAL_MAP: {
            struct ObjMap* orig_map = value->as.map_type;
            struct ObjMap* map = make_map();
            push_root(to_map(map));
            map->default_value = orig_map->default_value;
            copy_table(&map->table, &orig_map->table);
            pop_root();
            return to_map(map);
        }
        case VAL_LIST: {
            struct ObjList* orig_list = value->as.list_type;
            
            struct ObjList* list = make_list();
            push_root(to_list(list));
            list->default_value = orig_list->default_value;
            copy_value_array(&list->values, &orig_list->values);
            pop_root();
            return to_list(list);
        }
        case VAL_STRING: {
            struct ObjString* orig_str = value->as.string_type;
            push_root(to_string(orig_str));
            struct ObjString* str = make_string(orig_str->chars, orig_str->length);
            pop_root();
            return to_string(str);
        }
        default:
            return *value;
    }
}


void init_value_array(struct ValueArray* va) {
    va->count = 0; 
    va->capacity = 0;
    va->values = ALLOCATE_ARRAY(Value);
}

//Only freeing the array - GC needs to take are of Objects
int free_value_array(struct ValueArray* va) {
    return FREE_ARRAY(va->values, Value, va->capacity);
}

int add_value(struct ValueArray* va, Value value) {
    if (va->count + 1 > va->capacity) {
        int new_capacity = va->capacity == 0 ? 8 : va->capacity * 2;
        va->values = GROW_ARRAY(va->values, Value, new_capacity, va->capacity);
        va->capacity = new_capacity;
    }

    va->values[va->count] = value;
    return va->count++;
}

void copy_value_array(struct ValueArray* dest, struct ValueArray* src) {
    free_value_array(dest);
    init_value_array(dest);
    for (int i = 0; i < src->count; i++) {
        add_value(dest, copy_value(&src->values[i]));
    }
}


