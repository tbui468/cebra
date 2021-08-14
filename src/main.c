#include "common.h"
#include "parser.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "decl_list.h"
#include "ast_typer.h"

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
 */

//TODO:
//
//  Refactor
//      - combine common uses into macros - especially the read_byte, read_string, etc in VM
//      - let parser peek ahead 4 (instead of just 1 currently) to clean up parsing logic
//          -separate decl/stmt/expr into own shit
//          - decl_list should be renamed to something else since it holds expressions, decls and stmts (does it?)
//      - keep track of allocated objects in a linked list for GC later
//      - what is the ALLOCATE macro used for?  Trace its usage.  We need to know for GC later.  Get rid of it/combine if necessary
//      - skim crafting interpreters (1st part with jlox) and test edge cases - then stamp them out or record them for later
//
//  Type checking - do the type checking at the same time at compiling!
//      - get the basic framework up and running with errors/warnings to user
//      - since compiling returns a void anyway, use the return value to return datatypes
//
//  Closures
//      could skip classes and just do structs if we have closures
//
//  Garbage Collection - need to keep linked list of all allocated objects (in vm makes the most sense)
//      add code inside memory.h - all allocations should go through there
//
//
//Need to remember to make GC for ObjStrings (all Objs)


ResultCode run_source(VM* vm, const char* source) {

    //create list of declarations (AST)
    DeclList dl;
    init_decl_list(&dl);

    ResultCode parse_result = parse(source, &dl);
    
    if (parse_result == RESULT_FAILED) {
        free_decl_list(&dl);
        return RESULT_FAILED;
    }

#ifdef DEBUG_AST
    print_decl_list(&dl);
#endif

    ResultCode run_result = compile_and_run(vm, &dl);

    if (run_result == RESULT_FAILED) {
        free_decl_list(&dl);
        return RESULT_FAILED; 
    }

    free_decl_list(&dl);

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

    ResultCode result = RESULT_SUCCESS;
    if (argc == 1) {
        result = repl();
    } else if (argc == 2) {
        result = run_script(argv[1]);
    }

    if (result == RESULT_FAILED) {
        return 1;
    }

    return 0;
}
