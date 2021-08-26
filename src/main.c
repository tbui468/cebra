#include "common.h"
#include "parser.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "node_list.h"
#include "memory.h"
#include "table.h"



ResultCode run_source(VM* vm, const char* source) {

    //create list of declarations (AST)
    NodeList nl;
    init_node_list(&nl);

    ResultCode parse_result = parse(source, &nl);

    if (parse_result == RESULT_FAILED) {
        free_node_list(&nl);
        return RESULT_FAILED;
    }

#ifdef DEBUG_AST
    print_node_list(&nl);
#endif

    ResultCode run_result = compile_and_run(vm, &nl);

    if (run_result == RESULT_FAILED) {
        free_node_list(&nl);
        return RESULT_FAILED; 
    }

    free_node_list(&nl);

    return RESULT_SUCCESS;
}


ResultCode repl() {
    VM vm;
    init_vm(&vm);
    int MAX_CHARS = 256;
    char input_line[MAX_CHARS];
    while(true) {
        printf("> ");
        fgets(input_line, MAX_CHARS, stdin);
        if (input_line[0] == 'q') {
            break;
        }

        //if not a complete statement expression, should keep reading

        run_source(&vm, &input_line[0]);
    }

    free_vm(&vm);

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

ResultCode run_script(const char* path) {
    VM vm;
    init_vm(&vm);

    const char* source = read_file(path);
    run_source(&vm, source);

    free((void*)source);
    free_vm(&vm);

    return RESULT_SUCCESS;
}

int main(int argc, char** argv) {

    init_memory_manager();

    /*
    ///////////testing Hash table//////////////////
    struct Table table;
    init_table(&table);
    struct ObjString* test1 = make_string("zebra", 5);
    Value v1 = to_integer(1);    
    struct ObjString* test2 = make_string("dog", 3);
    Value v2 = to_integer(2);    
    struct ObjString* test3 = make_string("turtle", 6);
    Value v3 = to_integer(3);    
    struct ObjString* test4 = make_string("fish", 4);
    Value v4 = to_integer(4);    
    struct ObjString* test5 = make_string("cat", 3);
    Value v5 = to_integer(5);    
    struct ObjString* test6 = make_string("lion", 4);
    Value v6 = to_integer(6);    
    struct ObjString* test7 = make_string("rabbit", 6);
    Value v7 = to_integer(7);    
    set_pair(&table, test1, v1);
    set_pair(&table, test2, v2);
    set_pair(&table, test3, v3);
    set_pair(&table, test4, v4);
    set_pair(&table, test5, v5);
    set_pair(&table, test6, v6);
    set_pair(&table, test7, v7);
    print_table(&table);

    Value value;

    if (get_value(&table, test7, &value)) {
        print_value(value);
    } else {
        printf("Not found");
    }

    free_table(&table);

    //////////end of hash table test//////////
    */

    ResultCode result = RESULT_SUCCESS;
    if (argc == 1) {
        result = repl();
    } else if (argc == 2) {
        result = run_script(argv[1]);
    }

    if (result == RESULT_FAILED) {
        return 1;
    }
    print_memory();
    return 0;
}
