#include "compiler.h"
#include "memory.h"
#include "object.h"

#define EMIT_TYPE(compiler, op_code, value) \
    emit_byte(compiler, op_code); \
    if (compiler->chunk->count + (int)sizeof(value) > compiler->chunk->capacity) { \
        grow_capacity(compiler); \
    } \
    memcpy(&compiler->chunk->codes[compiler->chunk->count], &value, sizeof(value)); \
    compiler->chunk->count += sizeof(value)

Sig* compile_node(Compiler* compiler, struct Node* node, SigList* ret_sigs);
static SigList compile_function(Compiler* compiler, NodeList* nl);

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


static Sig* compile_literal(Compiler* compiler, struct Node* node) {
    Literal* literal = (Literal*)node;
    switch(literal->name.type) {
        case TOKEN_INT: {
            int32_t integer = (int32_t)strtol(literal->name.start, NULL, 10);
            EMIT_TYPE(compiler, OP_INT, integer);
            return make_prim_sig(VAL_INT);
        }
        case TOKEN_FLOAT: {
            double f = strtod(literal->name.start, NULL);
            EMIT_TYPE(compiler, OP_FLOAT, f);
            return make_prim_sig(VAL_FLOAT);
        }
        case TOKEN_STRING: {
            ObjString* str = make_string(literal->name.start, literal->name.length);
            EMIT_TYPE(compiler, OP_STRING, str);
            return make_prim_sig(VAL_STRING);
        }
        case TOKEN_TRUE: {
            emit_byte(compiler, OP_TRUE);
            return make_prim_sig(VAL_BOOL);
        }
        case TOKEN_FALSE: {
            emit_byte(compiler, OP_FALSE);
            return make_prim_sig(VAL_BOOL);
        }
    }
}

static Sig* compile_unary(Compiler* compiler, struct Node* node) {
    Unary* unary = (Unary*)node;
    Sig* sig = compile_node(compiler, unary->right, NULL);
    emit_byte(compiler, OP_NEGATE);
    return sig;
}

static Sig* compile_logical(Compiler* compiler, struct Node* node) {
    Logical* logical = (Logical*)node;

    if (logical->name.type == TOKEN_AND) {
        Sig* left_type = compile_node(compiler, logical->left, NULL);
        int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
        Sig* right_type = compile_node(compiler, logical->right, NULL);
        patch_jump(compiler, false_jump);
        if (!same_sig(left_type, right_type)) {
            add_error(compiler, logical->name, "Left and right types must match.");
        }
        free_sig(left_type);
        free_sig(right_type);
        return make_prim_sig(VAL_BOOL);
    }

    if (logical->name.type == TOKEN_OR) {
        Sig* left_type = compile_node(compiler, logical->left, NULL);
        int true_jump = emit_jump(compiler, OP_JUMP_IF_TRUE);
        emit_byte(compiler, OP_POP);
        Sig* right_type = compile_node(compiler, logical->right, NULL);
        patch_jump(compiler, true_jump);
        if (!same_sig(left_type, right_type)) {
            add_error(compiler, logical->name, "Left and right types must match.");
        }
        free_sig(left_type);
        free_sig(right_type);
        return make_prim_sig(VAL_BOOL);
    }


    Sig* left_type = compile_node(compiler, logical->left, NULL);
    Sig* right_type = compile_node(compiler, logical->right, NULL);
    if (!same_sig(left_type, right_type)) {
        add_error(compiler, logical->name, "Left and right types must match.");
    }
    free_sig(left_type);
    free_sig(right_type);
    switch(logical->name.type) {
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
    return make_prim_sig(VAL_BOOL);
}

static Sig* compile_binary(Compiler* compiler, struct Node* node) {
    Binary* binary = (Binary*)node;

    Sig* type1 = compile_node(compiler, binary->left, NULL);
    Sig* type2 = compile_node(compiler, binary->right, NULL);
    if (!same_sig(type1, type2)) {
        add_error(compiler, binary->name, "Left and right types must match.");
    }
    free_sig(type2); //returning type1, so not freeing
    switch(binary->name.type) {
        case TOKEN_PLUS: emit_byte(compiler, OP_ADD); break;
        case TOKEN_MINUS: emit_byte(compiler, OP_SUBTRACT); break;
        case TOKEN_STAR: emit_byte(compiler, OP_MULTIPLY); break;
        case TOKEN_SLASH: emit_byte(compiler, OP_DIVIDE); break;
        case TOKEN_MOD: emit_byte(compiler, OP_MOD); break;
    }

    return type1;
}

static Sig* compile_print(Compiler* compiler, struct Node* node) {
    Print* print = (Print*)node;
    Sig* type = compile_node(compiler, print->right, NULL);
    free_sig(type);
    emit_byte(compiler, OP_PRINT);
    return make_prim_sig(VAL_NIL);
}

static void set_local(Compiler* compiler, Token name, Sig* sig, int index) {
    Local local;
    local.name = name;
    local.sig = sig;
    local.depth = compiler->scope_depth;
    compiler->locals[index] = local;
}

static void add_local(Compiler* compiler, Token name, Sig* sig) {
    set_local(compiler, name, sig, compiler->locals_count);
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


static Sig* compile_node(Compiler* compiler, struct Node* node, SigList* ret_sigs) {
    if (node == NULL) {
        return make_prim_sig(VAL_NIL);
    }
    switch(node->type) {
        //declarations
        case NODE_DECL_VAR: {
            DeclVar* dv = (DeclVar*)node;
            Sig* sig = compile_node(compiler, dv->right, NULL);
            add_local(compiler, dv->name, dv->sig);
            if (!sig_is_type(sig, VAL_NIL) && !same_sig(sig, dv->sig)) {
                add_error(compiler, dv->name, "Declaration type and right hand side type must match.");
            }
            free_sig(sig);
            return make_prim_sig(VAL_NIL);
        }
        case NODE_DECL_FUN: {
            DeclFun* df = (DeclFun*)node;

            printf("Function Decl sig: ");
            print_sig(df->sig);
            printf("\n");

            //adding function signatures for type checking
            int arity = df->parameters.count;
            add_local(compiler, df->name, df->sig);

            Compiler func_comp;
            Chunk chunk;
            init_chunk(&chunk);
            init_compiler(&func_comp, &chunk);
            func_comp.enclosing = (struct Compiler*)compiler;

            //set local slot 0 in new compiler to function definition (so recursion can work)
            //Recall: locals are only used for compiler to sync local variables with vm
            set_local(&func_comp, df->name, df->sig, 0);

            add_node(&df->parameters, df->body);
            SigList inner_ret_sigs = compile_function(&func_comp, &df->parameters);

            SigFun* sigfun = (SigFun*)df->sig;
            for (int i = 0; i < inner_ret_sigs.count; i++) {
                if (!same_sig(sigfun->ret, inner_ret_sigs.sigs[i])) {
                    add_error(compiler, df->name, "Return type must match signature in function declaration.");
                }
            }

            free_sig_list(&inner_ret_sigs);

            ObjFunction* f = make_function(chunk, arity);
            EMIT_TYPE(compiler, OP_FUN, f);
            free_compiler(&func_comp);
            return make_prim_sig(VAL_NIL);
        }
        //statements
        case NODE_PRINT:    return compile_print(compiler, node);
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            start_scope(compiler);
            for (int i = 0; i < block->decl_list.count; i++) {
                Sig* sig = compile_node(compiler, block->decl_list.nodes[i], ret_sigs); //TODO: 
                free_sig(sig);
            }
            end_scope(compiler);
            return make_prim_sig(VAL_NIL);
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            Sig* cond = compile_node(compiler, ie->condition, ret_sigs);
            if (!sig_is_type(cond, VAL_BOOL)) {
                add_error(compiler, ie->name, "Condition must evaluate to boolean.");
            }
            free_sig(cond);

            int jump_then = emit_jump(compiler, OP_JUMP_IF_FALSE); 

            emit_byte(compiler, OP_POP);
            Sig* then_sig = compile_node(compiler, ie->then_block, ret_sigs);
            free_sig(then_sig);
            int jump_else = emit_jump(compiler, OP_JUMP);

            patch_jump(compiler, jump_then);
            emit_byte(compiler, OP_POP);
            Sig* else_sig = compile_node(compiler, ie->else_block, ret_sigs);
            free_sig(else_sig);
            patch_jump(compiler, jump_else);
            return make_prim_sig(VAL_NIL);
        }
        case NODE_WHILE: {
            While* wh = (While*)node;
            int start = compiler->chunk->count;
            Sig* cond = compile_node(compiler, wh->condition, ret_sigs);
            if (!sig_is_type(cond, VAL_BOOL)) {
                add_error(compiler, wh->name, "Condition must evaluate to boolean.");
            }
            free_sig(cond);

            int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);

            emit_byte(compiler, OP_POP);
            Sig* then_block = compile_node(compiler, wh->then_block, ret_sigs);
            free_sig(then_block);

            //+3 to include OP_JUMP_BACK and uint16_t for jump distance
            int from = compiler->chunk->count + 3;
            emit_jump_by(compiler, OP_JUMP_BACK, from - start);
            patch_jump(compiler, false_jump);   
            emit_byte(compiler, OP_POP);
            return make_prim_sig(VAL_NIL);
        }
        case NODE_FOR: {
            For* fo = (For*)node;
            Sig* init = compile_node(compiler, fo->initializer, ret_sigs); //should leave no value on stack
            free_sig(init);

            int condition_start = compiler->chunk->count;
            Sig* cond = compile_node(compiler, fo->condition, ret_sigs);
            if (!sig_is_type(cond, VAL_BOOL)) {
                add_error(compiler, fo->name, "Condition must evaluate to boolean.");
            }
            free_sig(cond);

            int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
            int body_jump = emit_jump(compiler, OP_JUMP);

            int update_start = compiler->chunk->count;
            if (fo->update) {
                Sig* up = compile_node(compiler, fo->update, ret_sigs);
                free_sig(up);
                emit_byte(compiler, OP_POP); //pop update
            }
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->chunk->count + 3 - condition_start);

            patch_jump(compiler, body_jump);
            emit_byte(compiler, OP_POP); //pop condition if true
            Sig* then_sig = compile_node(compiler, fo->then_block, ret_sigs);
            free_sig(then_sig);
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->chunk->count + 3 - update_start);

            patch_jump(compiler, exit_jump);
            emit_byte(compiler, OP_POP); //pop condition if false
            return make_prim_sig(VAL_NIL);
        }
        case NODE_RETURN: {
            Return* ret = (Return*)node;
            Sig* sig = compile_node(compiler, ret->right, ret_sigs);
            emit_byte(compiler, OP_RETURN);
            add_sig(ret_sigs, copy_sig(sig));
            return sig;
        }
        //expressions
        case NODE_LITERAL:      return compile_literal(compiler, node);
        case NODE_BINARY:       return compile_binary(compiler, node);
        case NODE_LOGICAL:      return compile_logical(compiler, node);
        case NODE_UNARY:        return compile_unary(compiler, node);
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;
            emit_byte(compiler, OP_GET_VAR);
            uint8_t idx = find_local(compiler, gv->name);
            emit_byte(compiler, idx);
            Sig* sig = copy_sig(compiler->locals[idx].sig);
            return sig;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            Sig* sig = compile_node(compiler, sv->right, ret_sigs);
            emit_byte(compiler, OP_SET_VAR);
            emit_byte(compiler, find_local(compiler, sv->name));
            if (sv->decl) {
                emit_byte(compiler, OP_POP);
            }
            return sig;
        }
        case NODE_CALL: {
            Call* call = (Call*)node;
            //push <fn> definition to top of stack
            emit_byte(compiler, OP_GET_VAR);
            emit_byte(compiler, find_local(compiler, call->name));

            //push arguments to top of stack and check types
            uint8_t idx = find_local(compiler, call->name);
            Sig* sig = compiler->locals[idx].sig;
            SigList* params = &((SigFun*)sig)->params;
            if (params->count != call->arguments.count) {
                add_error(compiler, call->name, "Argument count must match declaration.");
            }

            int min = call->arguments.count < params->count ? call->arguments.count : params->count;
            for (int i = 0; i < min; i++) {
                Sig* arg_sig = compile_node(compiler, call->arguments.nodes[i], ret_sigs);
                if (!same_sig(arg_sig, params->sigs[i])) {
                    add_error(compiler, call->name, "Argument type must match parameter type.");
                }
                free_sig(arg_sig);
            }

            //make a new callframe
            emit_byte(compiler, OP_CALL);
            emit_byte(compiler, (uint8_t)(call->arguments.count));
            return copy_sig(((SigFun*)sig)->ret);
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

void free_compiler(Compiler* compiler) {
}

static SigList compile_function(Compiler* compiler, NodeList* nl) {
    SigList ret_sigs;
    init_sig_list(&ret_sigs);
    for (int i = 0; i < nl->count; i++) {
        Sig* sig = compile_node(compiler, nl->nodes[i], &ret_sigs);
        free_sig(sig);
    }

    emit_byte(compiler, OP_RETURN);
    return ret_sigs;
}

ResultCode compile_ast(Compiler* compiler, NodeList* nl) {
    SigList sl = compile_function(compiler, nl);
    free_sig_list(&sl);

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

