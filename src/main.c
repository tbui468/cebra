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

static ResultCode run_source(VM* vm, struct Compiler* script_comp, const char* source) {

    struct NodeList* final_ast;
    //passing globals and nodes in here feels messy - couldn't we have parse CREATE them?
    //Note: script_comp.nodes are ALL nodes, whereas nl only contains top statement nodes
    //printf("before parsing\n");
    ResultCode parse_result = parse(source, &final_ast, &script_comp->globals, &script_comp->nodes);

    if (parse_result == RESULT_FAILED) {
        return RESULT_FAILED;
    }

#ifdef DEBUG_AST
    print_node(final_ast);
#endif

    //printf("before compiling\n");
    ResultCode compile_result = compile_script(script_comp, final_ast);

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

static ResultCode run_script(VM* vm, const char* path) {

    const char* source = read_file(path);

    //passing in NULL for struct Type* bc compiler needs to be initialized
    //before Types can be created
    struct Compiler script_comp;
    init_compiler(&script_comp, "script", 6, 0, make_dummy_token(), NULL);
    define_native_functions(&script_comp);

    ResultCode result = run_source(vm, &script_comp, source);

    //free open_upvalues and stack so that GC can reclaim memory
    vm->open_upvalues = NULL;
    pop_stack(vm);

    free_compiler(&script_comp);

    free((void*)source);

    return result;
}

static ResultCode repl(VM* vm) {
    printf("Cebra 0.0.1\nType 'quit()' to exit the repl.\n");

    char input_line[MAX_CHARS];

    int current = 0;

    struct Compiler script_comp;
    init_compiler(&script_comp, "script", 6, 0, make_dummy_token(), NULL);
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
