#include "common.h"
#include "parser.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "memory.h"
#include "table.h"
#include "obj.h"



ResultCode run_source(VM* vm, const char* source) {

    printf("Before init script compiler\n");
    struct Compiler script_compiler;
    init_compiler(&script_compiler, "script", 6, 0);

    printf("Before parser\n");
    struct Node* nl = make_node_list();
    ResultCode parse_result = parse(source, (struct NodeList*)nl);
    printf("After parser\n");

    if (parse_result == RESULT_FAILED) {
        free_compiler(&script_compiler);
        return RESULT_FAILED;
    }

#ifdef DEBUG_AST
    print_node(nl);
#endif

    printf("Before compiler\n");
    ResultCode compile_result = compile_script(&script_compiler, (struct NodeList*)nl);
    printf("After compiler\n");

    if (compile_result == RESULT_FAILED) {
        printf("Compilation Failed\n");
        free_compiler(&script_compiler);
        return RESULT_FAILED; 
    }

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(script_compiler.function);
    int i = 0;
    printf("-------------------\n");
    while (i < script_compiler.function->chunk.count) {
       OpCode op = script_compiler.function->chunk.codes[i++];
       printf("[ %s ]\n", op_to_string(op));
    }
#endif

    //need to free script compiler, since need to pop two temporaries from root_stack
    free_compiler(&script_compiler);

    printf("Before run\n");
    ResultCode run_result = run(vm, script_compiler.function);
    printf("After run\n");

    if (run_result == RESULT_FAILED) {
        free_compiler(&script_compiler);
        return RESULT_FAILED; 
    }



    return RESULT_SUCCESS;
}


ResultCode repl(VM* vm) {
    int MAX_CHARS = 256;
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

    VM vm;
    init_vm(&vm);

    init_memory_manager(&vm);

    ResultCode result = RESULT_SUCCESS;
    if (argc == 1) {
        result = repl(&vm);
    } else if (argc == 2) {
        result = run_script(&vm, argv[1]);
    }

#ifdef DEBUG_STRESS_GC
    collect_garbage();
#endif

    free_vm(&vm);
    free_memory_manager();

    print_memory();

    if (result == RESULT_FAILED) {
        return 1;
    }

    return 0;
}
