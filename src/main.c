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

//TODO:
//function names add_int, add_float, add_function is compiler is a bit unclear
//  those are being added to the chunk opcodes (+ constants).  Could I give them a clearer name?
//
//When that function object is called (found on the stack using the locals in the enclosing function)
//We grab the function object from the stack (at the given local slot)
//and then push it into the VM callframes stack
//the callframes have an ordering (since it IS a stack) but they may overlap
//
//  The VM needs to maintain a callframe stack
//  DeclFun needs to create function object on the stack
//
//  need to compiler DeclFun into a Value on the stack that we can instantiate later
//  add_local
//
//  split compilers into linked list, where top compiler has enclosed = NULL
//  all inner function/compilers have enclosing = outer compiler
//
//  finish decl_or_assignment in parser.c (making function declaration node)
//      store function objects pointers on the stack (as locals)
//          is a hash table really necessary???
//
//  Each function call is its own compiler???
//  What's the problem we need to solve?
//      Declare functions
//      Call functions
//
//  Closures
//      could skip classes and just do structs if we have closures
//
//  Garbage Collection
//
//  Type checking - do the type checking at the same time at comiling!
//      since compiling returns a void anyway, use the return value to return types
//      We only need to traverse AST once this way
//
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
