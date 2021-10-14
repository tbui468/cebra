#include "common.h"
#include "parser.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "memory.h"
#include "table.h"
#include "obj.h"


#define MAX_CHARS 256

ResultCode run_source(VM* vm, const char* source) {

    struct Compiler script_comp;
    //passing in NULL for struct Type* bc compiler needs to be initialized
    //before Types can be created
    init_compiler(&script_comp, "script", 6, 0, make_dummy_token(), NULL);

    struct NodeList* nl;
    //passing globals and nodes in here feels messy - couldn't we have parse CREATE them?
    //Note: script_comp.nodes are ALL nodes, whereas nl only contains top statement nodes
    ResultCode parse_result = parse(source, &nl, &script_comp.globals, &script_comp.nodes);

    if (parse_result == RESULT_FAILED) {
        free_compiler(&script_comp);
        return RESULT_FAILED;
    }

#ifdef DEBUG_AST
    print_node(nl);
#endif

    ResultCode compile_result = compile_script(&script_comp, nl);

    if (compile_result == RESULT_FAILED) {
        printf("Compilation Failed\n");
        free_compiler(&script_comp);
        return RESULT_FAILED; 
    }

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(script_comp.function);
    int i = 0;
    printf("-------------------\n");
    while (i < script_comp.function->chunk.count) {
       OpCode op = script_comp.function->chunk.codes[i++];
       printf("[ %s ]\n", op_to_string(op));
    }
#endif

    //need to free script compiler, since need to pop two temporaries from root_stack
    free_compiler(&script_comp);

    //pop_roots here - no longer need to pop_roots
    ResultCode run_result = run(vm, script_comp.function);

    if (run_result == RESULT_FAILED) {
        return RESULT_FAILED; 
    }



    return RESULT_SUCCESS;
}


ResultCode repl(VM* vm) {
    char input_line[MAX_CHARS];
    while(true) {
        printf("> ");
        fgets(input_line, MAX_CHARS, stdin);
        if (input_line[0] == 'q') {
            break;
        }

        //if not a complete statement expression, should keep reading

        run_source(vm, &input_line[0]);
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
    ResultCode result = run_source(vm, source);

    free((void*)source);

    return result;
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
    //that keys/values can be collected
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
