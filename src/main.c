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



/*
static ResultCode define_open(struct Compiler* compiler) {
    struct Type* str_type = make_string_type();
    struct Type* params = make_type_array();
    add_type((struct TypeArray*)params, str_type);
    return define_native(compiler, "open", open_native, make_fun_type(params, make_file_type()));
}*/

static Value clock_native(int arg_count, Value* args) {
    return to_float((double)clock() / CLOCKS_PER_SEC);
}

static Value print_native(int arg_count, Value* args) {
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
    return to_nil();
}

static Value input_native(int arg_count, Value* args) {
    char input[100];
    fgets(input, 100, stdin);
    return to_string(make_string(input, strlen(input) - 1));
}

static ResultCode define_input(struct Compiler* compiler) {
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_string_type());
    return define_native(compiler, "input", input_native, make_fun_type(make_type_array(), returns));
}

static ResultCode define_clock(struct Compiler* compiler) {
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_float_type());
    return define_native(compiler, "clock", clock_native, make_fun_type(make_type_array(), returns));
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
    define_clock(&script_comp);
    define_print(&script_comp);
    define_input(&script_comp);

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
    define_clock(&script_comp);
    define_print(&script_comp);
    define_input(&script_comp);

    //TODO: reading all input into same buffer is temporary placeholder
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

//#ifdef DEBUG_STRESS_GC
    collect_garbage();
//#endif

    free_vm(&vm);

    free_memory_manager();

    print_memory();

    if (result == RESULT_FAILED) {
        return 1;
    }

    return 0;
}
