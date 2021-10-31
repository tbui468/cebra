#include "common.h"
#include "parser.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "memory.h"
#include "table.h"
#include "obj.h"
#include "native.h"

#define MAX_CHARS 256 * 256

//parse and return updated intermediate results (including Tokens for imports)
//  then all the read_file calls can happen here too
//process all intermediate results (resolving identifiers, inheritance, and ordering of static/dynamic code)
//compile
//run

/*
static ResultCode run_source(VM* vm, struct Compiler* script_comp, const char* source) {

    ResultCode result = RESULT_SUCCESS;

    struct NodeList* final_ast = (struct NodeList*)make_node_list();
    result = parse(source, final_ast, &script_comp->globals, &script_comp->nodes, script_comp->function->name);

    //process AST nodes here??? - recall that the ultimate goal is to be able to free script char*

#ifdef DEBUG_AST
    print_node(final_ast);
#endif

    if (result != RESULT_FAILED) result = compile_script(script_comp, final_ast);

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(script_comp->function);
    int i = 0;
    printf("-------------------\n");
    while (i < script_comp->function->chunk.count) {
       OpCode op = script_comp->function->chunk.codes[i++];
       printf("[ %s ]\n", op_to_string(op));
    }
#endif

    if (result != RESULT_FAILED) result = run(vm, script_comp->function);

    return result;
}*/

ResultCode read_file(const char* path, const char** source) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return RESULT_FAILED;
    }
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(file_size + 1);
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);
    *source = buffer;
    return RESULT_SUCCESS;
}

static ResultCode run_script(VM* vm, const char* path) {

    const char* source;
    if (read_file(path, &source) == RESULT_FAILED) {
        printf("[Cebra Error] File not found.\n");
        return RESULT_FAILED;
    }

    //passing in NULL for struct Type* bc compiler needs to be initialized
    //before Types can be created
    struct Compiler script_comp;
    init_compiler(&script_comp, path, strlen(path), 0, make_dummy_token(), NULL);
    define_native_functions(&script_comp);

    //run source would need to pass in 
    /* ///////////////TODO: Old stuff///////////////////////////
    ResultCode result = RESULT_SUCCESS;
    struct NodeList* final_ast = (struct NodeList*)make_node_list();
    result = parse(source, final_ast, &script_comp.globals, &script_comp.nodes, script_comp.function->name);
    if (result != RESULT_FAILED) result = compile_script(&script_comp, final_ast);
    if (result != RESULT_FAILED) result = run(vm, script_comp.function);*/;
    ///////////////////////////////////////////////////////////

    /////////////new stuff//////////////////////////////
    //parse root script
    ResultCode result = RESULT_SUCCESS;
    struct NodeList* final_ast = (struct NodeList*)make_node_list();
    struct NodeList* static_nodes = (struct NodeList*)make_node_list();
    struct NodeList* dynamic_nodes = (struct NodeList*)make_node_list();
    Token imports[64];
    int import_count = 0; //same as sources
    const char* sources[64];
    result = parse_new(source, static_nodes, dynamic_nodes, &script_comp.globals, imports, &import_count);

    //parse modules
    //TODO: don't parse module if it has already been parsed once (to eliminate circular dependencies)
    //TODO: free module source code (add it to a list of current sources, and free at the very end)
    char* last_slash = strrchr(path, DIR_SEPARATOR);
    int dir_len = last_slash == NULL ? 0 : last_slash - path + 1;

    for (int i = 0; i < import_count; i++) {
        Token import_name = imports[i];
        char* module_path = (char*)malloc(dir_len + import_name.length + 5); //current script directory + module name + '.cbr' extension and null terminator
        memcpy(module_path, path, dir_len);
        memcpy(module_path + dir_len, import_name.start, import_name.length);
        memcpy(module_path + dir_len + import_name.length, ".cbr\0", 5);
        const char* module_source; //TODO: this guy(s) needs to be freed after script/repl is done running
        //if (read_file(module_path, &module_source) == RESULT_FAILED) {
        if (read_file(module_path, &sources[i]) == RESULT_FAILED) {
            printf("[Cebra Error] Module not found.\n");
            result = RESULT_FAILED;
        }

        if (result != RESULT_FAILED) result = parse_new(sources[i], static_nodes, dynamic_nodes, &script_comp.globals, imports, &import_count);

        free(module_path);
    }

    //process AST, compile, and run
    if (result != RESULT_FAILED) result = process_ast(static_nodes, dynamic_nodes, &script_comp.globals, script_comp.nodes, final_ast);
    if (result != RESULT_FAILED) result = compile_script(&script_comp, final_ast);
    if (result != RESULT_FAILED) result = run(vm, script_comp.function);
    ///////////////////////////////////////////////////

    //free open_upvalues and stack so that GC can reclaim memory
    vm->open_upvalues = NULL;
    pop_stack(vm);

    free_compiler(&script_comp);

    free((void*)source);
    for (int i = 0; i < import_count; i++) {
        free((void*)sources[i]);
    }

    return result;
}

static ResultCode repl(VM* vm) {
    printf("Cebra 0.0.1\nType 'quit()' to exit the repl.\n");

    char input_line[MAX_CHARS];

    int current = 0;

    struct Compiler script_comp;
    init_compiler(&script_comp, "repl", 4, 0, make_dummy_token(), NULL);
    define_native_functions(&script_comp);

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

        ResultCode result = RESULT_SUCCESS;
        struct NodeList* final_ast = (struct NodeList*)make_node_list();
        result = parse(&input_line[current], final_ast, &script_comp.globals, &script_comp.nodes, script_comp.function->name);
        if (result != RESULT_FAILED) result = compile_script(&script_comp, final_ast);
        if (result != RESULT_FAILED) result = run(vm, script_comp.function);

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
