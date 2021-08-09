#include "common.h"
#include "parser.h"
#include "ast.h"
#include "compiler.h"
#include "value.h"
#include "vm.h"
#include "decl_list.h"

/*
 * Parser -> Typer -> Compiler -> VM
 */

//TODO:
//test.cbr freezes right now after two print statements - what's happening?
//  Why is parser get_next() requires advance() two times to get past double quotes?
//  need to redo lexer into something more readable
//
//StatementList should be DeclList - all programs are a list of declarations
//  decl - classDecl | funDecl | varDecl | stmt
//  stmt - exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block
//      stmts leave nothing on the stack when done
//      expressions leave one new value on the stack when done
//      what about decl??? no? - but they add to constants/globals
//
//Integrate DeclList with the rest of the pipeline
//
//Make grow_capacity in chunk a terniary statement instead of it's own function 
//
//move ast printing to a separate print_ast or debug file
//
//split parser and lexer up (if it's easy to do) so that there's less murk
//
//What we want: "this" + " dog" should output "this dog"
//work from the beginning parse ->
//get strings working
//  when do we need the hash table????
//  we need them for String objects
//  what are string literals??
//


ResultCode run(VM* vm, char* source) {

    //create AST from source
    DeclList dl;
    init_decl_list(&dl);
    //Expr* ast;
    //ResultCode parse_result = parse(source, &ast);
    ResultCode parse_result = parse(source, &dl);

    if (parse_result == RESULT_FAILED) {
        free_decl_list(&dl);
        return RESULT_FAILED;
    }

    print_decl_list(&dl);
    printf("\n");

/* 

    //type check ast
    ResultCode type_result = type_check(ast);

    if (type_result == RESULT_FAILED) {
        free_expr(ast);
        return RESULT_FAILED;
    }

    
    //compile ast to byte code
    //will want to compile to function object (script) later, which the vm will run
    Chunk chunk;
    init_chunk(&chunk);
    ResultCode compile_result = compile(ast, &chunk);

    if (compile_result == RESULT_FAILED) {
        free_chunk(&chunk);
        free_expr(ast);
       return RESULT_FAILED; 
    }

    disassemble_chunk(&chunk);

    //execute code on vm
    ResultCode run_result = execute(vm, &chunk);

    if (run_result == RESULT_FAILED) {
        free_chunk(&chunk);
        free_expr(ast);
       return RESULT_FAILED; 
    }

    free_chunk(&chunk);
    free_expr(ast);*/
    free_decl_list(&dl);

    return RESULT_SUCCESS;
}


ResultCode repl() {
    VM vm;
    vm_init(&vm);
    int MAX_CHARS = 256;
    char input_line[MAX_CHARS];
    while(true) {
        printf("> ");
        fgets(input_line, MAX_CHARS, stdin);
        if (input_line[0] == 'q') {
            break;
        }

        //if not a complete statement expression, should keep reading

        run(&vm, &input_line[0]);
    }

    vm_free(&vm);

    return RESULT_SUCCESS;
}

char* read_file(const char* path) {
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
    vm_init(&vm);

    char* source = read_file(path);
    run(&vm, source);

    free(source);
    vm_free(&vm);

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
