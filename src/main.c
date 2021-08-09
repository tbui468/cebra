#include "common.h"
#include "parser.h"
#include "ast.h"
#include "compiler.h"
#include "value.h"
#include "vm.h"
#include "decl_list.h"
#include "ast_typer.h"

/*
 * Parser -> Typer -> Compiler -> VM
 */


/*
 * AST structure
 *
//StatementList should be DeclList - all programs are a list of declarations
//  decl - classDecl | funDecl | varDecl | stmt
//  stmt - exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block
//      stmts leave nothing on the stack when done
//      expressions leave one new value on the stack when done
//      what about decl??? no? - but they add to constants/globals
 */

//TODO:
//ObjStrings work with compiler
//  Get them working with vm to print put string concatenations etc
//ObjString - how should strings be stored?
//What we want: "this" + " dog" should output "this dog"
//get strings working
//  work from the beginning with parsing a string literal
//  What do we want to do with string literals?
//  when do we need the hash table???? Start with the API to the hash table: where do we call it?  What arguments need to be passed in?
//


ResultCode run(VM* vm, const char* source) {

    //create list of declarations (AST)
    DeclList dl;
    init_decl_list(&dl);

    ResultCode parse_result = parse(source, &dl);
    
    if (parse_result == RESULT_FAILED) {
        free_decl_list(&dl);
        return RESULT_FAILED;
    }

    print_decl_list(&dl);



    //type check
    ResultCode type_result = type_check(&dl);

    if (type_result == RESULT_FAILED) {
        free_decl_list(&dl);
        return RESULT_FAILED;
    }

    
    //compile ast to byte code
    //will want to compile to function object (script) later, which the vm will run
    Chunk chunk;
    init_chunk(&chunk);
    ResultCode compile_result = compile(&dl, &chunk);

    if (compile_result == RESULT_FAILED) {
        free_chunk(&chunk);
        free_decl_list(&dl);
       return RESULT_FAILED; 
    }

    disassemble_chunk(&chunk);

    //execute code on vm
    ResultCode run_result = execute(vm, &chunk);

    if (run_result == RESULT_FAILED) {
        free_chunk(&chunk);
        free_decl_list(&dl);
       return RESULT_FAILED; 
    }

    free_chunk(&chunk);
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
    vm_init(&vm);

    const char* source = read_file(path);
    run(&vm, source);

    free((void*)source);
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
