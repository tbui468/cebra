#include "compiler.h"
#include "memory.h"

Compiler compiler;
void compile_expr(Expr* expr);


static void add_byte(uint8_t byte) {
    //expand capacity if necessary
    if (compiler.chunk->count + 1 > compiler.chunk->capacity) {
        int new_capacity = compiler.chunk->capacity == 0 ? 8 : compiler.chunk->capacity * 2;
        compiler.chunk->codes = GROW_ARRAY(OpCode, compiler.chunk->codes, new_capacity);
        compiler.chunk->capacity = new_capacity;
    }

    compiler.chunk->codes[compiler.chunk->count] = byte;
    compiler.chunk->count++;
}

static void add_bytes(uint8_t byte1, uint8_t byte2) {
    add_byte(byte1);
    add_byte(byte2);
}


static int add_constant(Value value) {
    compiler.chunk->constants[compiler.chunk->constants_idx] = value;
    compiler.chunk->constants_idx++;
    return compiler.chunk->constants_idx - 1;
}

static void compile_literal(Expr* expr) {
    Literal* literal = (Literal*)expr;
    switch(literal->name.type) {
        case TOKEN_INT: {
            int num = (int)strtol(literal->name.start, NULL, 10);
            int idx = add_constant(to_integer(num));
            add_bytes(OP_INT, idx); 
            break;
        }
        case TOKEN_FLOAT: {
            double num = strtod(literal->name.start, NULL);
            int idx = add_constant(to_float(num));
            add_bytes(OP_FLOAT, idx); 
            break;
        }
        //default:            add_error(literal->name, "Unrecognized token."); break;
    }
}

static void compile_unary(Expr* expr) {
    Unary* unary = (Unary*)expr;
    compile_expr(unary->right);
    add_byte(OP_NEGATE);
}

static void compile_binary(Expr* expr) {
    Binary* binary = (Binary*)expr;
    compile_expr(binary->left);
    compile_expr(binary->right);
    switch(binary->name.type) {
        case TOKEN_PLUS: add_byte(OP_ADD); break;
        case TOKEN_MINUS: add_byte(OP_SUBTRACT); break;
        case TOKEN_STAR: add_byte(OP_MULTIPLY); break;
        case TOKEN_SLASH: add_byte(OP_DIVIDE); break;
    }
}

static void compile_print(Expr* expr) {
    Print* print = (Print*)expr;
    compile_expr(print->right);
    add_byte(OP_PRINT);
}

static void compile_expr(Expr* expr) {
    switch(expr->type) {
        case EXPR_LITERAL:  compile_literal(expr); break;
        case EXPR_BINARY:   compile_binary(expr); break;
        case EXPR_UNARY:    compile_unary(expr); break;
        case EXPR_PRINT:    compile_print(expr); break;
    } 

}


void init_chunk(Chunk* chunk) {
    chunk->codes = ALLOCATE_ARRAY(OpCode);
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->constants_idx = 0;
}


void free_chunk(Chunk* chunk) {
    FREE(chunk->codes);
}

ResultCode compile(DeclList* dl, Chunk* chunk) {
    compiler.chunk = chunk;

    for (int i = 0; i < dl->count; i++) {
        compile_expr(dl->decls[i]);
    }
    add_byte(OP_RETURN);

    return RESULT_SUCCESS;
}

static int get_int(int idx) {
    return compiler.chunk->constants[idx].as.integer_type;
}

static float get_float(int idx) {
    return compiler.chunk->constants[idx].as.float_type;
}

void disassemble_chunk(Chunk* chunk) {
    int i = 0;
    while (i < chunk->count) {
        switch(chunk->codes[i]) {
            case OP_INT: 
                i++;
                printf("[OP_INT] %d %d\n", chunk->codes[i], get_int(chunk->codes[i])); 
                break;
            case OP_FLOAT: 
                i++;
                printf("[OP_FLOAT] %d %f\n", chunk->codes[i], get_float(chunk->codes[i])); 
                break;
            case OP_ADD: printf("[OP_ADD]\n"); break;
            case OP_SUBTRACT: printf("[OP_SUBTRACT]\n"); break;
            case OP_MULTIPLY: printf("[OP_MULTIPLY]\n"); break;
            case OP_DIVIDE: printf("[OP_DIVIDE]\n"); break;
            case OP_NEGATE: printf("[OP_NEGATE]\n"); break;
            case OP_PRINT: printf("[OP_PRINT]\n"); break;
            case OP_RETURN:
                printf("[OP_RETURN]\n");
                break;
            default: printf("other\n"); break;
        }
        i++;
    }
}
