#include "common.h"
#include "parser.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "memory.h"
#include "table.h"
#include "obj.h"
#include "native.h"

#define MAX_IMPORTS 256
#define MAX_SOURCES 1024
#define MAX_CHARS_PER_LINE 512
#define MODULE_DIR_PATH "C:\\dev\\cebra\\examples\\interpreter_using_modules\\"

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

//sources and script counts won't match up since script count is reset

static ResultCode run_source(VM* vm, const char** sources, int* source_count, struct Compiler* script_comp, const char* modules_dir_path) {
    ResultCode result = RESULT_SUCCESS;

    Token scripts[MAX_IMPORTS];
    int script_count = 0;
    struct NodeList* static_nodes = (struct NodeList*)make_node_list();
    struct NodeList* dynamic_nodes = (struct NodeList*)make_node_list();
    if (result != RESULT_FAILED) result = parse(sources[*source_count - 1], static_nodes, dynamic_nodes, &script_comp->globals, scripts, &script_count);

    int modules_dir_len = strlen(modules_dir_path);

    for (int i = 0; i < script_count; i++) {
        Token script_name = scripts[i];
        char* module_path = (char*)malloc(modules_dir_len + script_name.length + 5); //modules directory + module name + '.cbr' extension and null terminator
        memcpy(module_path, modules_dir_path, modules_dir_len);
        memcpy(module_path + modules_dir_len, script_name.start, script_name.length);
        memcpy(module_path + modules_dir_len + script_name.length, ".cbr\0", 5);
        if (read_file(module_path, &sources[*source_count]) == RESULT_FAILED) {
            printf("[Cebra Error] Module not found.\n");
            result = RESULT_FAILED;
        }

        if (result != RESULT_FAILED) {
            result = parse(sources[*source_count], static_nodes, dynamic_nodes, &script_comp->globals, scripts, &script_count);
            *source_count = *source_count + 1;
        }

        free(module_path);
    }

    //process AST, compile, and run
    struct NodeList* final_ast = (struct NodeList*)make_node_list();
    if (result != RESULT_FAILED) result = process_ast(static_nodes, dynamic_nodes, &script_comp->globals, script_comp->nodes, final_ast);
    if (result != RESULT_FAILED) result = compile_script(script_comp, final_ast);
    if (result != RESULT_FAILED) result = run(vm, script_comp->function);

    return result;
}

static ResultCode run_script(VM* vm, const char* root_script_path) {
    ResultCode result = RESULT_SUCCESS;

    //passing in NULL for struct Type* bc compiler needs to be initialized
    //before Types can be created
    struct Compiler script_comp;
    init_compiler(&script_comp, root_script_path, strlen(root_script_path), 0, make_dummy_token(), NULL);
    define_native_functions(&script_comp);

    int source_count = 1; //first source file will be the root script
    const char* sources[64];

    if (read_file(root_script_path, &sources[0]) == RESULT_FAILED) {
        printf("[Cebra Error] Module not found.\n");
        result = RESULT_FAILED;
    }

    //use script path as the module root path
    char* last_slash = strrchr(root_script_path, DIR_SEPARATOR);
    int dir_len = last_slash == NULL ? 0 : last_slash - root_script_path + 1;
    char* module_dir_path = (char*)malloc(dir_len + 1);
    memcpy(module_dir_path, root_script_path, dir_len);
    module_dir_path[dir_len] = '\0';

    //TODO: this won't work in the repl since we are looping through the sources from the beginning everytime
    if (result != RESULT_FAILED) result = run_source(vm, sources, &source_count, &script_comp, module_dir_path);

    //free open_upvalues and stack so that GC can reclaim memory
    vm->open_upvalues = NULL;
    pop_stack(vm);

    free_compiler(&script_comp);

    free(module_dir_path);

    for (int i = 0; i < source_count; i++) {
        free((void*)sources[i]);
    }

    return result;
}

static ResultCode repl(VM* vm) {
    printf("Cebra 0.0.1\nType 'quit()' to exit the repl.\n");

    struct Compiler script_comp;
    init_compiler(&script_comp, "repl", 4, 0, make_dummy_token(), NULL);
    define_native_functions(&script_comp);

    //need to run once to add native functions to vm globals table, otherwise they won't be defined if user
    //code fails - the ip is set back to 0 and the chunk is also reset to 0 after running each line.
    struct NodeList* nl = (struct NodeList*)make_node_list();
    compile_script(&script_comp, nl); //need to do this so that OP_HALT is emitted
    run(vm, script_comp.function);
    script_comp.function->chunk.count = 0;

    int source_count = 0;
    char* sources[MAX_SOURCES];

    while(true) {
        ResultCode result = RESULT_SUCCESS;
        printf(">>> ");

        sources[source_count] = (char*)malloc(MAX_CHARS_PER_LINE); //max chars in line
        fgets(sources[source_count], MAX_CHARS_PER_LINE, stdin);
        source_count++;
        if (memcmp(sources[source_count - 1], "quit()", 6) == 0) {
            break;
        }

        const char* module_dir_path = "";
        if (result != RESULT_FAILED) result = run_source(vm, sources, &source_count, &script_comp, module_dir_path);

        //this resets vm instructions chunk in compiler back to 0 for next read
        script_comp.function->chunk.count = 0;
    }

    vm->open_upvalues = NULL;
    pop_stack(vm);

    free_compiler(&script_comp);

    for (int i = 0; i < source_count; i++) {
        free((void*)sources[i]);
    }

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
