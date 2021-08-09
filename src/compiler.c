#include "compiler.h"
#include "memory.h"

Compiler compiler;
void compile_expr(Expr* expr);

static void grow_capacity() {
    int new_capacity = compiler.chunk->capacity == 0 ? 8 : compiler.chunk->capacity * 2;
    compiler.chunk->codes = GROW_ARRAY(OpCode, compiler.chunk->codes, new_capacity);
    compiler.chunk->capacity = new_capacity;
}

static void add_byte(uint8_t byte) {
    if (compiler.chunk->count + 1 > compiler.chunk->capacity) {
        grow_capacity();
    }

    compiler.chunk->codes[compiler.chunk->count] = byte;
    compiler.chunk->count++;
}

static void add_int(uint8_t op_code, int32_t num) {
    if (compiler.chunk->count + (int)sizeof(int32_t) + 1 > compiler.chunk->capacity) {
        grow_capacity();
    }

    add_byte(op_code);

    memcpy(&compiler.chunk->codes[compiler.chunk->count], &num, sizeof(int32_t));
    compiler.chunk->count += sizeof(int32_t);
}

static void add_float(uint8_t op_code, double num) {
    if (compiler.chunk->count + (int)sizeof(double) + 1 > compiler.chunk->capacity) {
        grow_capacity();
    }

    add_byte(op_code);

    memcpy(&compiler.chunk->codes[compiler.chunk->count], &num, sizeof(double));
    compiler.chunk->count += sizeof(double);
}

static void compile_literal(Expr* expr) {
    Literal* literal = (Literal*)expr;
    switch(literal->name.type) {
        case TOKEN_INT: {
            add_int(OP_INT, (int32_t)strtol(literal->name.start, NULL, 10));
            break;
        }
        case TOKEN_FLOAT: {
            add_float(OP_FLOAT, strtod(literal->name.start, NULL));
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

static int32_t read_int(Chunk* chunk, int offset) {
    int32_t* ptr = (int32_t*)(&chunk->codes[offset]);
    return *ptr;
}

static double read_float(Chunk* chunk, int offset) {
    double* ptr = (double*)(&chunk->codes[offset]);
    return *ptr;
}

void disassemble_chunk(Chunk* chunk) {
    int i = 0;
    while (i < chunk->count) {
        switch(chunk->codes[i]) {
            case OP_INT: 
                printf("[OP_INT] %d\n", read_int(chunk, i + 1)); 
                i += sizeof(int32_t);
                break;
            case OP_FLOAT: 
                printf("[OP_FLOAT] %f\n", read_float(chunk, i + 1)); 
                i += sizeof(double);
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
