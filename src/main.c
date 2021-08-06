#include "common.h"
#include "parser.h"

//get the vm running with repl and files for a simple calculator
//
//group with pratt parser not working as intended.....
//once it works, make the pratt parser output and AST

//Parser -> AST
//  how to get AST Expr nodes passed around??? maybe google this
//  practice this to make sure it works (can cast between base Expr and Literal, Binary, etc) before trying to create AST
//  how can the visitor pattern be implemented in C?
//print AST to check if it works
//Typer -> AST
//Compiler -> Bytecode
//VM -> execute

typedef enum {
    RESULT_SUCCESS,
    RESULT_FAILED
} ResultCode;

/*
typedef enum {
    EXPR_LITERAL,
    EXPR_UNARY,
    EXPR_GROUPING,
    EXPR_BINARY,
} ExprType;

typedef struct {
    ExprType type;
} Expr;

typedef struct {
    Expr base;
} Literal;*/


typedef struct {
} VM;

ResultCode vm_init(VM* vm) {
}

ResultCode vm_free(VM* vm) {
}



ResultCode run(VM* vm, char* source) {

    //create AST from source
    parse(source);

    //type check

    //compile to byte code

    //execute code



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
