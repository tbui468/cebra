#include "compiler.h"
#include "memory.h"
#include "object.h"

void compile_node(Compiler* compiler, struct Node* node);

static void add_error(Compiler* compiler, Token token, const char* message) {
    CompileError error;
    error.token = token;
    error.message = message;
    compiler->errors[compiler->error_count] = error;
    compiler->error_count++;
}

static void grow_capacity(Compiler* compiler) {
    int new_capacity = compiler->chunk->capacity == 0 ? 8 : compiler->chunk->capacity * 2;
    compiler->chunk->codes = GROW_ARRAY(compiler->chunk->codes, OpCode, new_capacity, compiler->chunk->capacity);
    compiler->chunk->capacity = new_capacity;
}

static void emit_byte(Compiler* compiler, uint8_t byte) {
    if (compiler->chunk->count + 1 > compiler->chunk->capacity) {
        grow_capacity(compiler);
    }

    compiler->chunk->codes[compiler->chunk->count] = byte;
    compiler->chunk->count++;
}


static int emit_jump(Compiler* compiler, OpCode op) {
    emit_byte(compiler, op);
    emit_byte(compiler, 0xff);
    emit_byte(compiler, 0xff);
    return compiler->chunk->count;
}

static void patch_jump(Compiler* compiler, int index) {
    uint16_t destination = (uint16_t)(compiler->chunk->count - index);
    memcpy(&compiler->chunk->codes[index - 2], &destination, sizeof(uint16_t));
}

static void emit_short(Compiler* compiler, uint16_t num) {
    if (compiler->chunk->count + (int)sizeof(int16_t) > compiler->chunk->capacity) {
        grow_capacity(compiler);
    }

    memcpy(&compiler->chunk->codes[compiler->chunk->count], &num, sizeof(int16_t));
    compiler->chunk->count += sizeof(int16_t);
}

static void emit_jump_by(Compiler* compiler, OpCode op, int index) {
    emit_byte(compiler, op);
    emit_short(compiler, (uint16_t)index);
}

static void emit_int(Compiler* compiler, uint8_t op_code, int32_t num) {
    if (compiler->chunk->count + (int)sizeof(int32_t) + 1 > compiler->chunk->capacity) {
        grow_capacity(compiler);
    }

    emit_byte(compiler, op_code);

    memcpy(&compiler->chunk->codes[compiler->chunk->count], &num, sizeof(int32_t));
    compiler->chunk->count += sizeof(int32_t);
}

static void emit_float(Compiler* compiler, uint8_t op_code, double num) {
    if (compiler->chunk->count + (int)sizeof(double) + 1 > compiler->chunk->capacity) {
        grow_capacity(compiler);
    }

    emit_byte(compiler, op_code);

    memcpy(&compiler->chunk->codes[compiler->chunk->count], &num, sizeof(double));
    compiler->chunk->count += sizeof(double);
}

static void emit_string(Compiler* compiler, uint8_t op_code, ObjString* obj) {
    if (compiler->chunk->count + (int)sizeof(ObjString*) + 1 > compiler->chunk->capacity) {
        grow_capacity(compiler);
    }

    emit_byte(compiler, op_code);

    memcpy(&compiler->chunk->codes[compiler->chunk->count], &obj, sizeof(ObjString*));
    compiler->chunk->count += sizeof(ObjString*);
}

static void emit_function(Compiler* compiler, uint8_t op_code, ObjFunction* obj) {
    if (compiler->chunk->count + (int)sizeof(ObjFunction*) + 1 > compiler->chunk->capacity) {
        grow_capacity(compiler);
    }

    emit_byte(compiler, op_code);

    memcpy(&compiler->chunk->codes[compiler->chunk->count], &obj, sizeof(ObjFunction*));
    compiler->chunk->count += sizeof(ObjFunction*);
}


static void compile_literal(Compiler* compiler, struct Node* node) {
    Literal* literal = (Literal*)node;
    switch(literal->name.type) {
        case TOKEN_INT: {
            emit_int(compiler, OP_INT, (int32_t)strtol(literal->name.start, NULL, 10));
            break;
        }
        case TOKEN_FLOAT: {
            emit_float(compiler, OP_FLOAT, strtod(literal->name.start, NULL));
            break;
        }
        case TOKEN_STRING: {
            emit_string(compiler, OP_STRING, make_string(literal->name.start, literal->name.length));
            break;
        }
        case TOKEN_TRUE: {
            emit_byte(compiler, OP_TRUE);
            break;
        }
        case TOKEN_FALSE: {
            emit_byte(compiler, OP_FALSE);
            break;
        }
    }
}

static void compile_unary(Compiler* compiler, struct Node* node) {
    Unary* unary = (Unary*)node;
    compile_node(compiler, unary->right);
    emit_byte(compiler, OP_NEGATE);
}

static void compile_binary(Compiler* compiler, struct Node* node) {
    Binary* binary = (Binary*)node;

    //Note: compiling order is different for logical operators
    //so pulling TOKEN_AND and TOKEN_OR out of switch statement
    //Could make a new node type for AND and OR - maybe useful
    //when we add static typing during the AST traversal
    if (binary->name.type == TOKEN_AND) {
        compile_node(compiler, binary->left);
        int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
        compile_node(compiler, binary->right);
        patch_jump(compiler, false_jump);
        return;
    }

    if (binary->name.type == TOKEN_OR) {
        compile_node(compiler, binary->left);
        int true_jump = emit_jump(compiler, OP_JUMP_IF_TRUE);
        emit_byte(compiler, OP_POP);
        compile_node(compiler, binary->right);
        patch_jump(compiler, true_jump);
        return;
    }


    compile_node(compiler, binary->left);
    compile_node(compiler, binary->right);
    switch(binary->name.type) {
        case TOKEN_PLUS: emit_byte(compiler, OP_ADD); break;
        case TOKEN_MINUS: emit_byte(compiler, OP_SUBTRACT); break;
        case TOKEN_STAR: emit_byte(compiler, OP_MULTIPLY); break;
        case TOKEN_SLASH: emit_byte(compiler, OP_DIVIDE); break;
        case TOKEN_MOD: emit_byte(compiler, OP_MOD); break;
        case TOKEN_LESS:
            emit_byte(compiler, OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emit_byte(compiler, OP_GREATER);
            emit_byte(compiler, OP_NEGATE);
            break;
        case TOKEN_GREATER:
            emit_byte(compiler, OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emit_byte(compiler, OP_LESS);
            emit_byte(compiler, OP_NEGATE);
            break;
        case TOKEN_EQUAL_EQUAL:
            emit_byte(compiler, OP_EQUAL);
            break;
        case TOKEN_BANG_EQUAL:
            emit_byte(compiler, OP_EQUAL);
            emit_byte(compiler, OP_NEGATE);
            break;
    }
}

static void compile_print(Compiler* compiler, struct Node* node) {
    Print* print = (Print*)node;
    compile_node(compiler, print->right);
    emit_byte(compiler, OP_PRINT);
}


static void add_local(Compiler* compiler, Token name) {
    Local local;
    local.name = name;
    local.depth = compiler->scope_depth;
    compiler->locals[compiler->locals_count] = local;
    compiler->locals_count++;
}

static uint8_t find_local(Compiler* compiler, Token name) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->name.length == name.length && memcmp(local->name.start, name.start, name.length) == 0) {
            return (uint8_t)i;
        }
    }

    add_error(compiler, name, "Local variable not declared.");
}


static void start_scope(Compiler* compiler) {
    compiler->scope_depth++;
}

static void end_scope(Compiler* compiler) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->depth == compiler->scope_depth) {
            compiler->locals_count--;
            emit_byte(compiler, OP_POP);
        }else{
            break;
        }
    }

    compiler->scope_depth--;
}


static void compile_node(Compiler* compiler, struct Node* node) {
    if (node == NULL) {
        return;
    }
    switch(node->type) {
        //declarations
        case NODE_DECL_VAR: {
            DeclVar* dv = (DeclVar*)node;
            compile_node(compiler, dv->right);
            add_local(compiler, dv->name);
            break;
        }
        case NODE_DECL_FUN: {
            DeclFun* df = (DeclFun*)node;
            add_local(compiler, df->name);

            //creating new compiler
            //and adding the function def at local slot 0
            Compiler func;
            Chunk chunk;
            init_chunk(&chunk);
            init_compiler(&func, &chunk);
            func.enclosing = (struct Compiler*)compiler;
            Local local;
            local.name = df->name;
            local.depth = func.scope_depth;
            func.locals[0] = local;

            int arity = df->parameters.count;
            //adding body to parameter DeclList so only one compile call is needed
            add_node(&df->parameters, df->body);
            compile(&func, &df->parameters);

            emit_function(compiler, OP_FUN, make_function(chunk, arity));
            break;
        }
        //statements
        case NODE_PRINT:    compile_print(compiler, node); break;
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            start_scope(compiler);
            for (int i = 0; i < block->decl_list.count; i++) {
                compile_node(compiler, block->decl_list.nodes[i]);
            }
            end_scope(compiler);
            break;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            compile_node(compiler, ie->condition);
            int jump_then = emit_jump(compiler, OP_JUMP_IF_FALSE); 

            emit_byte(compiler, OP_POP);
            compile_node(compiler, ie->then_block);
            int jump_else = emit_jump(compiler, OP_JUMP);

            patch_jump(compiler, jump_then);
            emit_byte(compiler, OP_POP);
            compile_node(compiler, ie->else_block);
            patch_jump(compiler, jump_else);
            break;
        }
        case NODE_WHILE: {
            While* wh = (While*)node;
            int start = compiler->chunk->count;
            compile_node(compiler, wh->condition);
            int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);

            emit_byte(compiler, OP_POP);
            compile_node(compiler, wh->then_block);

            //+3 to include OP_JUMP_BACK and uint16_t for jump distance
            int from = compiler->chunk->count + 3;
            emit_jump_by(compiler, OP_JUMP_BACK, from - start);
            patch_jump(compiler, false_jump);   
            emit_byte(compiler, OP_POP);
            break;
        }
        case NODE_FOR: {
            For* fo = (For*)node;
            compile_node(compiler, fo->initializer); //should leave no value on stack

            int condition_start = compiler->chunk->count;
            compile_node(compiler, fo->condition);
            int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
            int body_jump = emit_jump(compiler, OP_JUMP);

            int update_start = compiler->chunk->count;
            if (fo->update) {
                compile_node(compiler, fo->update);
                emit_byte(compiler, OP_POP); //pop update
            }
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->chunk->count + 3 - condition_start);

            patch_jump(compiler, body_jump);
            emit_byte(compiler, OP_POP); //pop condition if true
            compile_node(compiler, fo->then_block);
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->chunk->count + 3 - update_start);

            patch_jump(compiler, exit_jump);
            emit_byte(compiler, OP_POP); //pop condition if false
            break;
        }
        case NODE_RETURN: {
            Return* ret = (Return*)node;
            compile_node(compiler, ret->right);
            emit_byte(compiler, OP_RETURN);
            break;
        }
        //expressions
        case NODE_LITERAL:  compile_literal(compiler, node); break;
        case NODE_BINARY:   compile_binary(compiler, node); break;
        case NODE_UNARY:    compile_unary(compiler, node); break;
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;
            emit_byte(compiler, OP_GET_VAR);
            emit_byte(compiler, find_local(compiler, gv->name));
            break;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            compile_node(compiler, sv->right);
            emit_byte(compiler, OP_SET_VAR);
            emit_byte(compiler, find_local(compiler, sv->name));
            if (sv->decl) {
                emit_byte(compiler, OP_POP);
            }
            break;
        }
        case NODE_CALL: {
            Call* call = (Call*)node;
            //push <fn> definition to top of stack
            emit_byte(compiler, OP_GET_VAR);
            emit_byte(compiler, find_local(compiler, call->name));
            //push arguments to top of stack
            for (int i = 0; i < call->arguments.count; i++) {
                compile_node(compiler, call->arguments.nodes[i]);
            }
            //make a new callframe
            emit_byte(compiler, OP_CALL);
            emit_byte(compiler, (uint8_t)(call->arguments.count));
            break;
        }
    } 
}

void init_compiler(Compiler* compiler, Chunk* chunk) {
    compiler->scope_depth = 0;
    compiler->locals_count = 1; //first slot is for function def
    compiler->error_count = 0;
    compiler->chunk = chunk;
    compiler->enclosing = NULL;
}

ResultCode compile(Compiler* compiler, NodeList* nl) {
    for (int i = 0; i < nl->count; i++) {
        compile_node(compiler, nl->nodes[i]);
    }

    emit_byte(compiler, OP_RETURN);

    if (compiler->error_count > 0) {
        for (int i = 0; i < compiler->error_count; i++) {
            printf("[line %d] %s\n", compiler->errors[i].token.line, compiler->errors[i].message);
        }
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}


void print_locals(Compiler* compiler) {
    for (int i = 0; i < compiler->locals_count; i ++) {
        Local* local = &compiler->locals[i];
        printf("%.*s\n", local->name.length, local->name.start);
    }
}

