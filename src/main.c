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
    struct NodeList* nl;
    ResultCode parse_result = parse(source, &nl, &script_compiler.globals);
    printf("After parser\n");

    if (parse_result == RESULT_FAILED) {
        free_compiler(&script_compiler);
        return RESULT_FAILED;
    }

#ifdef DEBUG_AST
    print_node(nl);
#endif

    printf("Before compiler\n");
    ResultCode compile_result = compile_script(&script_compiler, nl);
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
    //pop_roots here - no longer need to pop_roots
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

    //Note: memory manage needs a pointer to vm,
    //and vm needs memory manager initialized before
    //it can be initialized - this ordering is important
    ////////////////
    init_memory_manager();
    VM vm;
    mm.vm = &vm;
    init_vm(&vm);
    mm.vm_globals_initialized = true;
    ////////////////
    

    //TODO: testin hash table
    /*
    Value v;
    struct ObjString* canine = make_string("Canine", 7);
    push_root(to_string(canine));
    struct ObjString* bird = make_string("Bird", 7);
    push_root(to_string(bird));
    struct ObjString* token = make_string("Token", 7);
    push_root(to_string(token));
    struct ObjString* cat = make_string("Cat", 7);
    push_root(to_string(cat));
    struct ObjString* dog = make_string("Dog", 7);
    push_root(to_string(dog));
    struct ObjString* animal = make_string("Animal", 7);
    push_root(to_string(animal));
    struct ObjString* zebra = make_string("Zebra", 7);
    push_root(to_string(zebra));

    struct Table test;
    init_table(&test);

    set_table(&test, zebra, to_nil());
    set_table(&test, canine, to_nil());
    set_table(&test, bird, to_nil());
    set_table(&test, token, to_nil());
    set_table(&test, cat, to_nil());
    set_table(&test, dog, to_nil());
    print_table(&test);
    set_table(&test, animal, to_nil());
    if (get_from_table(&test, canine, &v)) printf("found canine!\n");
    if (get_from_table(&test, bird, &v)) printf("found bird!\n");
    if (get_from_table(&test, token, &v)) printf("found token!\n");
    if (get_from_table(&test, cat, &v)) printf("found cat!\n");
    if (get_from_table(&test, dog, &v)) printf("found dog!\n");
    if (get_from_table(&test, animal, &v)) printf("found animal!\n");
    if (get_from_table(&test, zebra, &v)) printf("found zebra!\n");

    print_table(&test);

    printf("**************************************\n");

    pop_root();
    pop_root();
    pop_root();
    pop_root();
    pop_root();
    pop_root();
    pop_root();*/

    ResultCode result = RESULT_SUCCESS;
    if (argc == 1) {
        result = repl(&vm);
    } else if (argc == 2) {
        result = run_script(&vm, argv[1]);
    }

    //Note: need to free table and set
    //vm_globals_initialized to false so
    //GC doesn't try to mark vm.globals
    //////////////////////////
    free_table(&vm.globals);
    mm.vm_globals_initialized = false;
    ////////////////////////////

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
