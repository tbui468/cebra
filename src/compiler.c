#include "compiler.h"
#include "memory.h"
#include "object.h"

Compiler compiler;

void compile_expr(Expr* expr);

static void add_error(Token token, const char* message) {
    CompileError error;
    error.token = token;
    error.message = message;
    compiler.errors[compiler.error_count] = error;
    compiler.error_count++;
}

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

static void add_string(uint8_t op_code, ObjString* obj) {
    if (compiler.chunk->count + (int)sizeof(ObjString*) + 1 > compiler.chunk->capacity) {
        grow_capacity();
    }

    add_byte(op_code);

    memcpy(&compiler.chunk->codes[compiler.chunk->count], &obj, sizeof(ObjString*));
    compiler.chunk->count += sizeof(ObjString*);
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
        case TOKEN_STRING: {
            add_string(OP_STRING, make_string(literal->name.start, literal->name.length));
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


static void add_local(Token name) {
    Local local;
    local.name = name;
    local.depth = compiler.scope_depth;
    compiler.locals[compiler.locals_count] = local;
    compiler.locals_count++;
}

static uint8_t find_local(Token name) {
    for (int i = compiler.locals_count - 1; i >= 0; i--) {
        Local* local = &compiler.locals[i];
        if (memcmp(local->name.start, name.start, name.length) == 0) {
            return (uint8_t)i;
        }
    }

    add_error(name, "Local variable not declared.");
}

/*
static void start_scope() {
    compiler.scope_depth++;
}

static void end_scope() {
    //find all locals at current depth and discard (move array pointer)
    int end = compiler.locals_count - 1;
    compiler.scope_depth--;
}*/

static void compile_decl_var(Expr* expr) {
    DeclVar* dv = (DeclVar*)expr;
    compile_expr(dv->right);

    //declared variable should be on top of vm stack at this point
    add_local(dv->name);
}

static void compile_expr(Expr* expr) {
    if (expr == NULL) {
        printf("Node pointer is null");
        return;
    }
    switch(expr->type) {
        case EXPR_LITERAL:  compile_literal(expr); break;
        case EXPR_BINARY:   compile_binary(expr); break;
        case EXPR_UNARY:    compile_unary(expr); break;
        case EXPR_DECL_VAR: compile_decl_var(expr); break;
        case EXPR_GET_VAR: {
            GetVar* gv = (GetVar*)expr;
            add_byte(OP_GET_VAR);
            add_byte(find_local(gv->name));
            break;
        }
        case EXPR_SET_VAR: {
            SetVar* sv = (SetVar*)expr;
            compile_expr(sv->right);
            add_byte(OP_SET_VAR);
            add_byte(find_local(sv->name));
            if (sv->decl) {
                add_byte(OP_POP);
            }
            break;
        }
        case EXPR_PRINT:    compile_print(expr); break;
    } 
}


void init_chunk(Chunk* chunk) {
    chunk->codes = ALLOCATE_ARRAY(OpCode);
    chunk->count = 0;
    chunk->capacity = 0;
}

static void init_compiler() {
    compiler.scope_depth = 0;
    compiler.locals_count = 0;
}


void free_chunk(Chunk* chunk) {
    FREE(chunk->codes);
}

ResultCode compile(DeclList* dl, Chunk* chunk) {
    init_compiler();
    compiler.chunk = chunk;

    for (int i = 0; i < dl->count; i++) {
        compile_expr(dl->decls[i]);
    }
    add_byte(OP_RETURN);

    if (compiler.error_count > 0) {
        for (int i = 0; i < compiler.error_count; i++) {
            printf("[line %d] %s\n", compiler.errors[i].token.line, compiler.errors[i].message);
        }
        return RESULT_FAILED;
    }

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

static ObjString* read_string(Chunk* chunk, int offset) {
    ObjString** ptr = (ObjString**)(&chunk->codes[offset]);
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
            case OP_STRING: 
                ObjString* obj = read_string(chunk, i + 1);
                printf("[OP_STRING] %s\n", obj->chars); 
                i += sizeof(ObjString*);
                break;
            case OP_ADD: printf("[OP_ADD]\n"); break;
            case OP_SUBTRACT: printf("[OP_SUBTRACT]\n"); break;
            case OP_MULTIPLY: printf("[OP_MULTIPLY]\n"); break;
            case OP_DIVIDE: printf("[OP_DIVIDE]\n"); break;
            case OP_NEGATE: printf("[OP_NEGATE]\n"); break;
            case OP_GET_VAR: {
                i++;
                uint8_t slot = chunk->codes[i];
                printf("[OP_GET_VAR] [%d]\n", slot);
                break;
            }
            case OP_SET_VAR: {
                i++;
                uint8_t slot = chunk->codes[i];
                printf("[OP_SET_VAR] [%d]\n", slot);
                break;
            }
            case OP_PRINT: printf("[OP_PRINT]\n"); break;
            case OP_POP: printf("[OP_POP]\n"); break;
            case OP_RETURN:
                printf("[OP_RETURN]\n");
                break;
            default: printf("other\n"); break;
        }
        i++;
    }
}
