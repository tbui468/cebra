#include "common.h"
#include "parser.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "node_list.h"
#include "memory.h"
#include "table.h"

/*
 * Parser -> Compiler -> VM
 *  compiler does both type checking and code generation during one AST traversal
 */

/*
 * AST structure
 *
//StatementList should be DeclList - all programs are a list of declarations
//  decl - classDecl | funDecl | varDecl | stmt
//      leave no effect on the stack
//  stmt - exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block
//      exprStmt are expressions where the result is discarded (pop the stack)
//  expr - Literal | Unary | Binary | SetVar | GetVar
//      push one value on the stack
 */

/*
 * Add correctness tests as features are implemented
 *  closures, structs
 */

/*
 * Closure test code
 *
//functions should capture locals where it's declared - top level in this case

a: int = 23

fun :: () -> int {
    -> a
}

print fun()

 */

//TODO:
//  Redo NODE_CASCADE in compiler to get it working
//
//  Currently there is no type checking for NODE_DECL_FUN, NODE_CALL in compiler using new defs table
//      reimplement type checking
//
//  Make a two pass compiler - the first pass compiles all function and struct declarations, and
//  the second pass defines them.  This allows function and struct declarations to be in any order.
//
//  Big problem: Can't access classes (or functions etc) outside the current function scope
//      so we can't instantiate or call functions (?) inside another function
//      check this assumption by writing some code to see if it runs
//
//      Yep, it doesn't work.
//
//      Two options: create closures and upvalue, or make a hash table and put all
//          class and function declarations as global objects.  Then we can access
//          them from anywhere.  Will have to rethink function pointers/first class
//          citizens if so.  
//
//          Each function has a table of global functions/class AND it has access to 
//          any outer functions tables.  This keeps any declared functions private.
//          
//          But what happens if a user tries to return one of those functions?  We let them.
//          Since it's just a reference to ObjFunction that can be pushed onto the local
//          stack and used until the function returns.
//
//  OP_INSTANCE in vm.c needs to be completed - look at notes in vm.c OP_INSTANCE
//      basic problem: how to go from ObjClass -> Table necessary for instance?
//
//  MONDAY
//  Make ObjInstance for storing instances
//  Add ObjInstance* to Value and any supporting functions
//  Check that sample class compiles
//  Make Table (hash table) for instances to store class fields and methods
//  Make dot notation work for getting/setting instance fields
//  Make methods callable using dot notation
//
//  Make NODE_DECL_CLASS (should take in Token name, and NodeList props)
//  test to see if it compiles
//  dot notation for OP_GET_PROP, OP_SET_PROP
//  Classes
//      Don't really need a closure, right? - Yes we do.  Since the class methods need to see instance fields
//      will need a hash table for struct properties
//      will need ObjInstance (each one should have own hash table)
//
//      Class declaration
//          Dog :: class {
//              a: int = 5
//              Dog :: () -> this {
//
//              }
//              b :: () -> {
//                  print a
//              }
//          }
//
//  Struct Getters and Setters
//      including type checking
//
//  Clear up the warnings (especially with switch statements) before doing garbage collection
//
//  in compiler, CALL and CASCADE CALL share a lot of code - pull that into a function
//
//  Having to call free_sig on almost all signatures in compile_node is a bit much
//      and super prone to errors
//
//  Garbage Collection - need to keep linked list of all allocated objects (in vm makes the most sense)
//      add code inside memory.h - all allocations should go through there
//      mark and sweep or reference counting?
//
//  Closures - nested functions and structs won't work unless these are implemented
//
//  Implement deleting from hash table - need to use tombstones
//
//  Intern strings - create a "strings" table in vm
//
//  Test Edge cases by writing toy programs - save these programs as correctness tests
//
//  Stress test by writing script to load nyc_subway data - compare runtime to python pandas
//
//  Write benchmarking code to use with chrome://tracing to find hotspots
//
//  Why are if /else so much slower than just if (think of the fibonacci example
//


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
