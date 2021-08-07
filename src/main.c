#include "common.h"
#include "parser.h"
#include "ast.h"

//Finish typer and parser
//  Parser and Typer should have an array of error codes we can access if RESULT_FAILED is returned
//  we don't need them printing out errors immediately
//Compiler -> Bytecode
//  Allocate memory for op codes array
//VM -> execute
//  allocate memory for vm stack
//  print stack for debugging
//
//need code to free AST too since the REPL may need to keep running


typedef struct {
} VM;

ResultCode vm_init(VM* vm) {
    return RESULT_SUCCESS;
}

ResultCode vm_free(VM* vm) {
    return RESULT_SUCCESS;
}



ResultCode run(VM* vm, char* source) {

    //create AST from source
    Expr* ast;
    ResultCode parse_result = parse(source, &ast);

    if (parse_result == RESULT_FAILED) {
        return RESULT_FAILED;
    }

    print_expr(ast);
    printf("\n");


    //type check ast
    ResultCode type_result = type_check(ast);

    if (type_result == RESULT_FAILED) {
        return RESULT_FAILED;
    }

    //compile ast to byte code

    //execute code on vm


    //free ast here

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
    printf("%s", source);
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
