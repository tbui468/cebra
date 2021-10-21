#include "common.h"
#include "parser.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "memory.h"
#include "table.h"
#include "obj.h"
#include <time.h>

#define MAX_CHARS 256 * 256

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

ResultCode run_source(VM* vm, struct Compiler* script_comp, const char* source) {

    struct NodeList* nl;
    //passing globals and nodes in here feels messy - couldn't we have parse CREATE them?
    //Note: script_comp.nodes are ALL nodes, whereas nl only contains top statement nodes
    //printf("before parsing\n");
    ResultCode parse_result = parse(source, &nl, &script_comp->globals, &script_comp->nodes);

    if (parse_result == RESULT_FAILED) {
        return RESULT_FAILED;
    }

#ifdef DEBUG_AST
    print_node(nl);
#endif

    //printf("before compiling\n");
    ResultCode compile_result = compile_script(script_comp, nl);

    if (compile_result == RESULT_FAILED) {
        return RESULT_FAILED; 
    }

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(script_comp->function);
    int i = 0;
    printf("-------------------\n");
    while (i < script_comp->function->chunk.count) {
       OpCode op = script_comp->function->chunk.codes[i++];
       printf("[ %s ]\n", op_to_string(op));
    }
#endif

    //printf("before running\n");
    ResultCode run_result = run(vm, script_comp->function);


    if (run_result == RESULT_FAILED) {
        return RESULT_FAILED; 
    }

    return RESULT_SUCCESS;
}



const char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(file_size + 1);
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

ResultCode run_script(VM* vm, const char* path) {

    const char* source = read_file(path);

    //passing in NULL for struct Type* bc compiler needs to be initialized
    //before Types can be created
    struct Compiler script_comp;
    init_compiler(&script_comp, "script", 6, 0, make_dummy_token(), NULL);
    define_print(&script_comp);
    define_clock(&script_comp);
    define_input(&script_comp);
    define_open(&script_comp);
    define_read_all(&script_comp);
    define_close(&script_comp);
    define_read_line(&script_comp);

    ResultCode result = run_source(vm, &script_comp, source);

    //free open_upvalues and stack so that GC can reclaim memory
    vm->open_upvalues = NULL;
    pop_stack(vm);

    free_compiler(&script_comp);

    free((void*)source);

    return result;
}

ResultCode repl(VM* vm) {
    printf("Cebra 0.0.1\nType 'quit()' to exit the repl.\n");

    char input_line[MAX_CHARS];

    int current = 0;

    struct Compiler script_comp;
    init_compiler(&script_comp, "script", 6, 0, make_dummy_token(), NULL);
    define_print(&script_comp);
    define_clock(&script_comp);
    define_input(&script_comp);
    define_open(&script_comp);
    define_read_all(&script_comp);
    define_close(&script_comp);
    define_read_line(&script_comp);

    //need to run once to add native functions to vm globals table, otherwise they won't be defined if user
    //code fails - the ip is set back to 0 and the chunk is also reset to 0 after running each line.
    struct NodeList* nl = (struct NodeList*)make_node_list();
    compile_script(&script_comp, nl); //need to do this so that OP_HALT is emitted
    run(vm, script_comp.function);
    script_comp.function->chunk.count = 0;

    while(true) {
        printf(">>> ");
        fgets(&input_line[current], MAX_CHARS - current, stdin);
        if (MAX_CHARS - current - (int)strlen(&input_line[current]) - 1 == 0) {
            printf("Exceeding repl max buffer size.  Exiting Cebra repl...\n");
            break;
        }

        if (memcmp(&input_line[current], "quit()", 6) == 0) {
            break;
        }


        run_source(vm, &script_comp, &input_line[current]);
        current += (int)strlen(&input_line[current]) + 1;

        //this resets vm instructions chunk in compiler back to 0 for next read
        script_comp.function->chunk.count = 0;
    }

    vm->open_upvalues = NULL;
    pop_stack(vm);

    free_compiler(&script_comp);

    return RESULT_SUCCESS;
}

int main(int argc, char** argv) {

    //VM needs memory manager initialized before
    //vm.strings/vm.globals tables can be initialized
    init_memory_manager();
    VM vm;
    mm.vm = &vm;
    init_vm(&vm);
    

    ResultCode result = RESULT_SUCCESS;
    if (argc == 1) {
        result = repl(&vm);
    } else if (argc == 2) {
        result = run_script(&vm, argv[1]);
    }

    //prevents GC from marking table so
    //that keys/values can be freed
    vm.initialized = false;

    collect_garbage();

    free_vm(&vm);

    free_memory_manager();

    print_memory();

    if (result == RESULT_FAILED) {
        return 1;
    }

    return 0;
}
