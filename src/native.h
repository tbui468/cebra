#ifndef NATIVE_H
#define NATIVE_H


#include <time.h>

//returns string, and is_eof boolean
static ResultCode read_line_native(int arg_count, Value* args, struct ValueArray* returns) {
    struct ObjFile* file = args[0].as.file_type;
    if (file->fp == NULL) {
        add_value(returns, to_nil());
        add_value(returns, to_nil());
        return RESULT_FAILED;
    }

    char input[256];
    char* result = fgets(input, 256, file->fp);
    if (result != NULL) {
        struct ObjString* s = make_string(input, strlen(input) - 1);
        push_root(to_string(s));
        add_value(returns, to_string(s));
        add_value(returns, to_boolean(false));
        pop_root();
    } else {
        struct ObjString* s = make_string("", 0);
        push_root(to_string(s));
        add_value(returns, to_string(s));
        add_value(returns, to_boolean(true));
        pop_root();
    }
    return RESULT_SUCCESS;
}

static ResultCode define_read_line(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_string_type());
    add_type(returns, make_bool_type());
    return define_native(compiler, "read_line", read_line_native, make_fun_type(params, returns));
}


static ResultCode close_native(int arg_count, Value* args, struct ValueArray* returns) {
    struct ObjFile* file = args[0].as.file_type;
    add_value(returns, to_nil());
    if (file->fp != NULL) {
        fclose(file->fp);
        file->fp = NULL;
        return RESULT_FAILED;
    }

    add_value(returns, to_nil());
    return RESULT_SUCCESS;
}

static ResultCode define_close(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    struct TypeArray* returns = make_type_array();
    return define_native(compiler, "close", close_native, make_fun_type(params, returns));
}

static ResultCode read_all_native(int arg_count, Value* args, struct ValueArray* returns) {
    FILE* fp = args[0].as.file_type->fp;
    if (fp == NULL) {
        add_value(returns, to_nil());
        return RESULT_FAILED;
    }

    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    rewind(fp);
    char* buffer = ALLOCATE_ARRAY(char);
    buffer = GROW_ARRAY(buffer, char, file_size + 1, 0);
    size_t bytes_read = fread(buffer, sizeof(char), file_size, fp);
    buffer[bytes_read] = '\0';
   
    struct ObjString* s = take_string(buffer, file_size);
    push_root(to_string(s)); 
    add_value(returns, to_string(s));
    pop_root();
    return RESULT_SUCCESS;
}

static ResultCode define_read_all(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    struct TypeArray* returns = make_type_array();
    struct Type* s_type = make_string_type();
    s_type->opt = make_nil_type();
    add_type(returns, s_type);
    return define_native(compiler, "read_all", read_all_native, make_fun_type(params, returns));
}

static ResultCode open_native(int arg_count, Value* args, struct ValueArray* returns) {
    FILE* fp;
    //TODO: opening in read mode only for now
    fp = fopen(args[0].as.string_type->chars, "r");
    if (fp == NULL) {
        add_value(returns, to_nil());
        return RESULT_FAILED;
    }

    struct ObjFile* file = make_file(fp);
    push_root(to_file(file));
    add_value(returns, to_file(file));
    pop_root();
    return RESULT_SUCCESS;
}


static ResultCode define_open(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_string_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_file_type());
    return define_native(compiler, "open", open_native, make_fun_type(params, returns));
}


static ResultCode input_native(int arg_count, Value* args, struct ValueArray* returns) {
    char input[256];
    fgets(input, 256, stdin);
    struct ObjString* s = make_string(input, strlen(input) - 1);
    push_root(to_string(s));
    add_value(returns, to_string(s));
    pop_root();
    return RESULT_SUCCESS;
}

static ResultCode define_input(struct Compiler* compiler) {
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_string_type());
    return define_native(compiler, "input", input_native, make_fun_type(make_type_array(), returns));
}

static ResultCode clock_native(int arg_count, Value* args, struct ValueArray* returns) {
    add_value(returns, to_float((double)clock() / CLOCKS_PER_SEC));
    return RESULT_SUCCESS;
}

static ResultCode define_clock(struct Compiler* compiler) {
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_float_type());
    return define_native(compiler, "clock", clock_native, make_fun_type(make_type_array(), returns));
}

static ResultCode print_native(int arg_count, Value* args, struct ValueArray* returns) {
    Value value = args[0];
    switch(value.type) {
        case VAL_STRING: {
            struct ObjString* str = value.as.string_type;
            printf("%.*s\n", str->length, str->chars);
            break;
        }
        case VAL_INT:
            printf("%d\n", value.as.integer_type);
            break;
        case VAL_FLOAT:
            printf("%f\n", value.as.float_type);
            break; 
        case VAL_NIL:
            printf("nil\n");
            break;
    }
    add_value(returns, to_nil());
    return RESULT_SUCCESS;
}

static ResultCode define_print(struct Compiler* compiler) {
    struct Type* str_type = make_string_type();
    struct Type* int_type = make_int_type();
    struct Type* float_type = make_float_type();
    struct Type* nil_type = make_nil_type();
    //using dummy token with enum type to allow printing enums (as integer)
    //otherwise the typechecker sees it as an error
    struct Type* enum_type = make_enum_type(make_dummy_token());
    str_type->opt = int_type;
    int_type->opt = float_type;
    float_type->opt = nil_type;
    nil_type->opt = enum_type;
    struct TypeArray* sl = make_type_array();
    add_type(sl, str_type);
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_nil_type());
    return define_native(compiler, "print", print_native, make_fun_type(sl, returns));
}

void define_native_functions(struct Compiler* compiler) {
    define_print(compiler);
    define_clock(compiler);
    define_input(compiler);
    define_open(compiler);
    define_read_all(compiler);
    define_close(compiler);
    define_read_line(compiler);
}


#endif
