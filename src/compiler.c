#include "compiler.h"
#include "memory.h"
#include "object.h"

Compiler compiler;

void compile_node(struct Node* node);

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


static void compile_literal(struct Node* node) {
    Literal* literal = (Literal*)node;
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
        case TOKEN_TRUE: {
            add_byte(OP_TRUE);
            break;
        }
        case TOKEN_FALSE: {
            add_byte(OP_FALSE);
            break;
        }
    }
}

static void compile_unary(struct Node* node) {
    Unary* unary = (Unary*)node;
    compile_node(unary->right);
    add_byte(OP_NEGATE);
}

static void compile_binary(struct Node* node) {
    Binary* binary = (Binary*)node;
    compile_node(binary->left);
    compile_node(binary->right);
    switch(binary->name.type) {
        case TOKEN_PLUS: add_byte(OP_ADD); break;
        case TOKEN_MINUS: add_byte(OP_SUBTRACT); break;
        case TOKEN_STAR: add_byte(OP_MULTIPLY); break;
        case TOKEN_SLASH: add_byte(OP_DIVIDE); break;
        case TOKEN_MOD: add_byte(OP_MOD); break;
        case TOKEN_LESS:
            add_byte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            add_byte(OP_GREATER);
            add_byte(OP_NEGATE);
            break;
        case TOKEN_GREATER:
            add_byte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            add_byte(OP_LESS);
            add_byte(OP_NEGATE);
            break;
    }
}

static void compile_print(struct Node* node) {
    Print* print = (Print*)node;
    compile_node(print->right);
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


static void start_scope() {
    compiler.scope_depth++;
}

static void end_scope() {
    for (int i = compiler.locals_count - 1; i >= 0; i--) {
        Local* local = &compiler.locals[i];
        if (local->depth == compiler.scope_depth) {
            compiler.locals_count--;
            add_byte(OP_POP);
        }else{
            break;
        }
    }

    compiler.scope_depth--;
}

static void compile_decl_var(struct Node* node) {
    DeclVar* dv = (DeclVar*)node;
    compile_node(dv->right);

    //declared variable should be on top of vm stack at this point
    add_local(dv->name);
}

static void compile_node(struct Node* node) {
    if (node == NULL) {
        printf("Node pointer is null");
        return;
    }
    switch(node->type) {
        //declarations
        case NODE_DECL_VAR: compile_decl_var(node); break;
        //statements
        case NODE_PRINT:    compile_print(node); break;
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            start_scope();
            for (int i = 0; i < block->decl_list.count; i++) {
                compile_node(block->decl_list.decls[i]);
            }
            end_scope();
            break;
        }
        case NODE_IF_ELSE: {
            //need to have booleans working first
            break;
        }
        //expressions
        case NODE_LITERAL:  compile_literal(node); break;
        case NODE_BINARY:   compile_binary(node); break;
        case NODE_UNARY:    compile_unary(node); break;
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;
            add_byte(OP_GET_VAR);
            add_byte(find_local(gv->name));
            break;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            compile_node(sv->right);
            add_byte(OP_SET_VAR);
            add_byte(find_local(sv->name));
            if (sv->decl) {
                add_byte(OP_POP);
            }
            break;
        }
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
        compile_node(dl->decls[i]);
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


const char* op_to_string(OpCode op) {
    switch(op) {
        case OP_INT: return "OP_INT";
        case OP_FLOAT: return "OP_FLOAT";
        case OP_STRING: return "OP_STRING";
        case OP_PRINT: return "OP_PRINT";
        case OP_SET_VAR: return "OP_SET_VAR";
        case OP_GET_VAR: return "OP_GET_VAR";
        case OP_ADD: return "OP_ADD";
        case OP_SUBTRACT: return "OP_SUBTRACT";
        case OP_MULTIPLY: return "OP_MULTIPLY";
        case OP_DIVIDE: return "OP_DIVIDE";
        case OP_MOD: return "OP_MOD";
        case OP_NEGATE: return "OP_NEGATE";
        case OP_POP: return "OP_POP";
        case OP_TRUE: return "OP_TRUE";
        case OP_FALSE: return "OP_FALSE";
        case OP_RETURN: return "OP_RETURN";
        default: return "Unrecognized op";
    }
}

void disassemble_chunk(Chunk* chunk) {
    int i = 0;
    while (i < chunk->count) {
        OpCode op = chunk->codes[i];
        printf("[ %s ] ", op_to_string(op));
        switch(op) {
            case OP_INT: 
                printf("%d", read_int(chunk, i + 1)); 
                i += sizeof(int32_t);
                break;
            case OP_FLOAT: 
                printf("%f", read_float(chunk, i + 1)); 
                i += sizeof(double);
                break;
            case OP_STRING: 
                ObjString* obj = read_string(chunk, i + 1);
                printf("%s", obj->chars); 
                i += sizeof(ObjString*);
                break;
            case OP_GET_VAR: {
                i++;
                uint8_t slot = chunk->codes[i];
                printf("[%d]", slot);
                break;
            }
            case OP_SET_VAR: {
                i++;
                uint8_t slot = chunk->codes[i];
                printf("[%d]", slot);
                break;
            }
        }
        printf("\n");
        i++;
    }
}
