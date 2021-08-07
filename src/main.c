#include "common.h"
#include "parser.h"
#include "ast.h"
#include "compiler.h"
#include "value.h"

//TODO:
//move vm to different .h and .c files
//free AST memory
//free chunk memory

//get running scripts working
//get booleans working
//get strings working
//  when do we need the hash table????
//  we need them for String objects
//  what are string literals??

typedef struct {
    Value stack[256];
    int stack_top;
    int ip;
} VM;

ResultCode vm_init(VM* vm) {
    vm->stack_top = 0;
    vm->ip = 0;
    return RESULT_SUCCESS;
}

ResultCode vm_free(VM* vm) {
    return RESULT_SUCCESS;
}

Value pop(VM* vm) {
    vm->stack_top--;
    return vm->stack[vm->stack_top];
}

void push(VM* vm, Value value) {
    vm->stack[vm->stack_top] = value;
    vm->stack_top++;
}

uint8_t read_byte(VM* vm, Chunk* chunk) {
    vm->ip++;
    return chunk->codes[vm->ip - 1]; 
}

Value get_constant(Chunk* chunk, int idx) {
    return chunk->constants[idx];
}

ResultCode execute(VM* vm, Chunk* chunk) {
   vm_init(vm); //TODO: need to think of better way to reset ip (but not necessarily the stack - for REPL)
   while (vm->ip < chunk->count) {
        switch(read_byte(vm, chunk)) {
            case OP_INT: {
                uint8_t idx = read_byte(vm, chunk);
                push(vm, get_constant(chunk, idx));
                break;
            }
            case OP_FLOAT: {
                uint8_t idx = read_byte(vm, chunk);
                push(vm, get_constant(chunk, idx));
                break;
            }
            case OP_NEGATE: {
                Value value = pop(vm);
                push(vm, negate_value(value));
                break;
            }
            case OP_ADD: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, add_values(a, b));
                break;
            }
            case OP_SUBTRACT: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, subtract_values(a, b));
                break;
            }
            case OP_MULTIPLY: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, multiply_values(a, b));
                break;
            }
            case OP_DIVIDE: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, divide_values(a, b));
                break;
            }
            case OP_RETURN: {
                Value value = pop(vm);
                if (value.type == VAL_INT) {
                    printf("%d\n", value.as.integer_type);
                } else {
                    printf("%f\n", value.as.float_type);
                }
                break;
            }
        } 
   } 

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
    Chunk chunk;
    chunk_init(&chunk);
    ResultCode compile_result = compile(ast, &chunk);

    disassemble_chunk(&chunk);

    //execute code on vm
    ResultCode run_result = execute(vm, &chunk);

    //free chunk here
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
