#include "common.h"
#include "parser.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "node_list.h"
#include "memory.h"

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

//TODO:
//
//  Need to change other compile_node subfunctions to return Sig* rather than ValueType
//          literal, binary, logical, unary, print
//
//  Generalize the sigs in compiler/parser to let them work with functions too (only primitives work now)
//      check by passing functions around using variables
//
//  Clear up the warnings (especially with switch statements)
//
//  Closures
//      first set up test.cbr function that captures a variable at declaration time
//      then write some code to get this working
//      each compiler needs a list of upvalues
//      need ObjUpvalue since we need to close some upvalues that are out of scope
//
//  Garbage Collection - need to keep linked list of all allocated objects (in vm makes the most sense)
//      add code inside memory.h - all allocations should go through there
//
//  Structs Instances
//
//  Struct Getters and Setters
//
//  Struct Instantiation
//
//  Struct Inheritance
//
//  Test Edge cases by writing toy programs - save these programs as correctness tests
//
//  Stress test by writing script to load nyc_subway data - compare runtime to python pandas
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
