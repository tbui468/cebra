#ifndef NATIVE_H
#define NATIVE_H

#include <time.h>
#include <ctype.h>
#include <math.h>

static ResultCode exp_native(Value* args, struct ValueArray* returns) {
    double pow;
    if (args[0].type == VAL_INT) {
        pow = (double)(args[0].as.integer_type);
    } else if (args[0].type == VAL_BYTE) {
        pow = (double)(args[0].as.byte_type);
    } else if (args[0].type == VAL_FLOAT) {
        pow = (double)(args[0].as.float_type);
    } else {
        return RESULT_FAILED;
    }

    add_value(returns, to_float(exp(pow)));

    return RESULT_SUCCESS;
}

static ResultCode define_exp(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    struct Type* f = make_float_type();
    struct Type* i = make_int_type();
    struct Type* b = make_byte_type();
    f->opt = i;
    i->opt = b;
    add_type(params, f);
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_float_type());
    return define_native(compiler, "exp", exp_native, make_fun_type(params, returns));
}

static ResultCode random_uniform_native(Value* args, struct ValueArray* returns) {
    double start = args[0].as.float_type;
    double end = args[1].as.float_type;
    double random_value = (double)rand()/RAND_MAX * (end - start) + start;
    add_value(returns, to_float(random_value));
    return RESULT_SUCCESS;
}

static ResultCode define_random_uniform(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_float_type());
    add_type(params, make_float_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_float_type());
    return define_native(compiler, "random_uniform", random_uniform_native, make_fun_type(params, returns));
}

static ResultCode is_digit_native(Value* args, struct ValueArray* returns) {
    struct ObjString* s = args[0].as.string_type;
    for (int i = 0; i < s->length; i++) {
        if (!isdigit(s->chars[i])) {
            add_value(returns, to_boolean(false));
            return RESULT_SUCCESS;
        }
    }

    add_value(returns, to_boolean(true));
    return RESULT_SUCCESS;
}

static ResultCode define_is_digit(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_string_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_bool_type());
    return define_native(compiler, "is_digit", is_digit_native, make_fun_type(params, returns));
}

static ResultCode is_alpha_native(Value* args, struct ValueArray* returns) {
    struct ObjString* s = args[0].as.string_type;
    for (int i = 0; i < s->length; i++) {
        if (!isalpha(s->chars[i])) {
            add_value(returns, to_boolean(false));
            return RESULT_SUCCESS;
        }
    }

    add_value(returns, to_boolean(true));
    return RESULT_SUCCESS;
}

static ResultCode define_is_alpha(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_string_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_bool_type());
    return define_native(compiler, "is_alpha", is_alpha_native, make_fun_type(params, returns));
}


static ResultCode append_string_with_escape_sequences(FILE* fp, char* s) {
    char* start = s;
    char* back_slash = strchr(start, '\\');
    while (back_slash != NULL) {
        fprintf(fp, "%.*s", (int)(back_slash-start), start);
        switch(*(back_slash + 1)) {
            case 'a': fprintf(fp, "\a"); break;
            case 'b': fprintf(fp, "\b"); break;
            case 'f': fprintf(fp, "\f"); break;
            case 'n': fprintf(fp, "\n"); break;
            case 'r': fprintf(fp, "\r"); break;
            case 't': fprintf(fp, "\t"); break;
            case 'v': fprintf(fp, "\v"); break;
            case '\\': fprintf(fp, "\\"); break;
            case '\'': fprintf(fp, "\'"); break;
            case '\"': fprintf(fp, "\""); break;
            case '?': fprintf(fp, "\?"); break;
        }
        start = back_slash + 2;
        back_slash = strchr(start, '\\');
    }

    fprintf(fp, "%s", start);

    return RESULT_SUCCESS;
}

static ResultCode append_native(Value* args, struct ValueArray* returns) {
    struct ObjFile* file = args[0].as.file_type;
    struct ObjString* s = args[1].as.string_type;
    if (fseek(file->fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "fseek() failed.");
        exit(1);
    }
    append_string_with_escape_sequences(file->fp, s->chars);

    fflush(file->fp);
    file->next_line = make_string("", 0);
    file->is_eof = true;
    add_value(returns, to_nil()); 
    return RESULT_SUCCESS;
}

static ResultCode define_append(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    add_type(params, make_string_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_nil_type());
    return define_native(compiler, "append", append_native, make_fun_type(params, returns));
}

static ResultCode process_next_line(struct ObjFile* file) {
    char input[256];
    char* result = fgets(input, 256, file->fp);
    if (result != NULL) {
        int len = input[strlen(input) - 1] == '\n' ? strlen(input) - 1 : strlen(input);
        file->next_line = make_string(input, len);
        file->is_eof = false;
    } else {
        file->next_line = make_string("", 0);
        file->is_eof = true;
    }

    return RESULT_SUCCESS;
}

static ResultCode rewind_native(Value* args, struct ValueArray* returns) {
    struct ObjFile* file = args[0].as.file_type;
    rewind(file->fp);
    file->is_eof = false;
    process_next_line(file);
    add_value(returns, to_nil()); 
    return RESULT_SUCCESS;
}

static ResultCode define_rewind(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_nil_type());
    return define_native(compiler, "rewind", rewind_native, make_fun_type(params, returns));
}

static ResultCode eof_native(Value* args, struct ValueArray* returns) {
    struct ObjFile* file = args[0].as.file_type;
    add_value(returns, to_boolean(file->is_eof)); 
    return RESULT_SUCCESS;
}

static ResultCode define_eof(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_bool_type());
    return define_native(compiler, "eof", eof_native, make_fun_type(params, returns));
}


static ResultCode read_line_native(Value* args, struct ValueArray* returns) {
    struct ObjFile* file = args[0].as.file_type;
    if (file->fp == NULL) {
        add_value(returns, to_nil());
        add_value(returns, to_nil());
        return RESULT_FAILED;
    }

    struct ObjString* line = file->next_line;
    push_root(to_string(line));

    process_next_line(file);

    add_value(returns, to_string(line));
    pop_root();

    return RESULT_SUCCESS;
}

static ResultCode define_read_line(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_string_type());
    return define_native(compiler, "read_line", read_line_native, make_fun_type(params, returns));
}


static ResultCode close_native(Value* args, struct ValueArray* returns) {
    struct ObjFile* file = args[0].as.file_type;
    add_value(returns, to_nil());
    if (file->fp != NULL) {
        fclose(file->fp);
        file->fp = NULL;
        return RESULT_SUCCESS;
    }

    return RESULT_FAILED;
}

static ResultCode define_close(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_nil_type());
    return define_native(compiler, "close", close_native, make_fun_type(params, returns));
}

static ResultCode read_all_bytes(Value* args, struct ValueArray* returns) {
    FILE* fp = args[0].as.file_type->fp;
    if (fp == NULL) {
        add_value(returns, to_nil());
        return RESULT_FAILED;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fprintf(stderr, "fseek() failed.");
        exit(1);
    }

    long file_size;
    if ((file_size = ftell(fp)) == -1L) {
        fprintf(stderr, "ftell() failed.");
        exit(1);
    }

    rewind(fp);
    char* buffer = ALLOCATE_ARRAY(char);
    buffer = GROW_ARRAY(buffer, char, file_size + 1, 0);

    long bytes_read = fread(buffer, sizeof(char), file_size, fp);
    if (bytes_read != file_size && feof(fp) == 0) {
        fprintf(stderr, "fread() failed.");
        exit(1);
    }

    buffer[bytes_read] = '\0';

    //make a byte from each char, and add to List
    struct ObjList* list = make_list();
    push_root(to_list(list));
    for (int i = 0; i < bytes_read; i++) {
        add_value(&list->values, to_byte((uint8_t)buffer[i]));
    }
    add_value(returns, to_list(list));
    pop_root(); 

    FREE_ARRAY(buffer, char, file_size + 1);

    return RESULT_SUCCESS;
}

static ResultCode define_read_bytes(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    struct TypeArray* returns = make_type_array();
    struct Type* byte_type = make_byte_type();
    struct Type* list_type = make_list_type(byte_type);
    add_type(returns, list_type);
    return define_native(compiler, "read_bytes", read_all_bytes, make_fun_type(params, returns));
}

static ResultCode read_all_native(Value* args, struct ValueArray* returns) {
    FILE* fp = args[0].as.file_type->fp;
    if (fp == NULL)
        exit(1);

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fprintf(stderr, "fseek() failed.");
        exit(1);
    }

    long file_size;
    if ((file_size = ftell(fp)) == -1L) {
        fprintf(stderr, "ftell() failed.");
        exit(1);
    }

    rewind(fp);
    char* buffer = ALLOCATE_ARRAY(char);
    buffer = GROW_ARRAY(buffer, char, file_size + 1, 0);

    long bytes_read = fread(buffer, sizeof(char), file_size, fp);
    if (bytes_read != file_size && feof(fp) == 0) {
        fprintf(stderr, "fread() failed.");
        exit(1);
    }

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

static ResultCode clear_native(Value* args, struct ValueArray* returns) {
    struct ObjFile* file = args[0].as.file_type;
    fclose(file->fp);

    //what the heck is going on here?    
    file->fp = fopen(file->file_path->chars, "w");
    if (file->fp == NULL) {
        fprintf(stderr, "fopen() failed.");
        exit(1);
    }
    fclose(file->fp);

    file->fp = fopen(file->file_path->chars, "a+");
    if (file->fp == NULL) {
        fprintf(stderr, "fopen() failed.");
        exit(1);
    }

    process_next_line(file);
    add_value(returns, to_nil());
    return RESULT_SUCCESS;
}

static ResultCode define_clear(struct Compiler* compiler) {
    struct TypeArray* params = make_type_array();
    add_type(params, make_file_type());
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_nil_type());
    return define_native(compiler, "clear", clear_native, make_fun_type(params, returns));
}


static ResultCode open_native(Value* args, struct ValueArray* returns) {
    FILE* fp;
    fp = fopen(args[0].as.string_type->chars, "a+");
    if (fp == NULL)
        exit(1);

    struct ObjFile* file = make_file(fp, args[0].as.string_type);
    push_root(to_file(file));
    process_next_line(file);
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


static ResultCode input_native(Value* args, struct ValueArray* returns) {
    char buffer[256];
    char* input = fgets(buffer, 256, stdin); //if NULL and feof(file) == 0
    if (input == NULL && feof(stdin) == 0) {
        fprintf(stderr, "fgets() failed.");
        exit(1);
    }
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

static ResultCode clock_native(Value* args, struct ValueArray* returns) {
    add_value(returns, to_float((double)clock() / CLOCKS_PER_SEC));
    return RESULT_SUCCESS;
}

static ResultCode define_clock(struct Compiler* compiler) {
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_float_type());
    return define_native(compiler, "clock", clock_native, make_fun_type(make_type_array(), returns));
}

static ResultCode print_string_with_escape_sequences(char* s) {
    char* start = s;
    char* back_slash = strchr(start, '\\');
    while (back_slash != NULL) {
        printf("%.*s", (int)(back_slash-start), start);
        switch(*(back_slash + 1)) {
            case 'a': printf("\a"); break;
            case 'b': printf("\b"); break;
            case 'f': printf("\f"); break;
            case 'n': printf("\n"); break;
            case 'r': printf("\r"); break;
            case 't': printf("\t"); break;
            case 'v': printf("\v"); break;
            case '\\': printf("\\"); break;
            case '\'': printf("\'"); break;
            case '\"': printf("\""); break;
            case '?': printf("\?"); break;
        }
        start = back_slash + 2;
        back_slash = strchr(start, '\\');
    }

    printf("%s", start);
    return RESULT_SUCCESS;
}

static ResultCode print_native(Value* args, struct ValueArray* returns) {
    Value value = args[0];
    switch(value.type) {
        case VAL_STRING: {
            struct ObjString* s = value.as.string_type;
            print_string_with_escape_sequences(s->chars);
            break;
        }
        case VAL_INT:
            printf("%d", value.as.integer_type);
            break;
        case VAL_BYTE:
            printf("%d", value.as.byte_type);
            break;
        case VAL_FLOAT:
            printf("%f", value.as.float_type);
            break; 
        case VAL_NIL:
            printf("nil");
            break;
        default:
            return RESULT_FAILED;
    }
    add_value(returns, to_nil());
    return RESULT_SUCCESS;
}

static ResultCode define_print(struct Compiler* compiler) {
    struct Type* str_type = make_string_type();
    struct Type* int_type = make_int_type();
    struct Type* byte_type = make_byte_type();
    struct Type* float_type = make_float_type();
    struct Type* nil_type = make_nil_type();
    //using dummy token with enum type to allow printing enums (as integer)
    //otherwise the typechecker sees it as an error
    struct Type* enum_type = make_enum_type(make_dummy_token());
    str_type->opt = int_type;
    int_type->opt = byte_type;
    byte_type->opt = float_type;
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
    define_read_bytes(compiler);
    define_close(compiler);
    define_read_line(compiler);
    define_eof(compiler);
    define_rewind(compiler);
    define_append(compiler);
    define_clear(compiler);
    define_is_alpha(compiler);
    define_is_digit(compiler);
    define_random_uniform(compiler);
    define_exp(compiler);
}


#endif
