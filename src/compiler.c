#include <stdio.h>
#include "compiler.h"
#include "memory.h"
#include "obj_string.h"
#include "obj_function.h"
#include "obj_class.h"


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

static int add_constant(Compiler* compiler, Value value) {
    add_value(&compiler->chunk->constants, value);
    return compiler->chunk->constants.count - 1;
}

static void emit_byte(Compiler* compiler, uint8_t byte) {
    if (compiler->chunk->count + 1 > compiler->chunk->capacity) {
        grow_capacity(compiler);
    }

    compiler->chunk->codes[compiler->chunk->count] = byte;
    compiler->chunk->count++;
}

static void emit_bytes(Compiler* compiler, uint8_t byte1, uint8_t byte2) {
    emit_byte(compiler, byte1);
    emit_byte(compiler, byte2);
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
            emit_bytes(compiler, OP_INT, add_constant(compiler, to_integer(integer)));
            return make_prim_sig(VAL_INT);
        }
        case TOKEN_FLOAT: {
            double f = strtod(literal->name.start, NULL);
            emit_bytes(compiler, OP_FLOAT, add_constant(compiler, to_float(f)));
            return make_prim_sig(VAL_FLOAT);
        }
        case TOKEN_STRING: {
            struct ObjString* str = make_string(literal->name.start, literal->name.length);
            emit_bytes(compiler, OP_STRING, add_constant(compiler, to_string(str)));
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

static Sig* find_sig(Compiler* compiler, Token name) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->name.length == name.length && memcmp(local->name.start, name.start, name.length) == 0) {
            return copy_sig(compiler->locals[i].sig);
        }
    }

    if (compiler->enclosing != NULL) {
        return find_sig((Compiler*)(compiler->enclosing), name);
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

            //class type
            if (dv->sig->type == SIG_CLASS) {
                SigClass* sc = (SigClass*)(dv->sig);
                emit_byte(compiler, OP_INSTANCE);
                uint8_t class_idx = find_local(compiler, sc->klass);
                emit_byte(compiler, class_idx);
                add_local(compiler, dv->name, dv->sig);

                //NOTE: don't need to check class types here
                //since we are looking up the class by
                //declaration type anyway, so they will
                //match or a "local not found" error will be added

                return make_prim_sig(VAL_NIL);
            }

            //primitive type
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

            //adding function signatures for type checking
            int arity = df->parameters.count;
            //add_local(compiler, df->name, df->sig);

            Compiler func_comp;
            Chunk chunk;
            init_chunk(&chunk);
            init_compiler(&func_comp, &chunk);
            func_comp.enclosing = (struct Compiler*)compiler;

            set_local(&func_comp, df->name, df->sig, 0);

            add_node(&df->parameters, df->body);
            SigList inner_ret_sigs = compile_function(&func_comp, &df->parameters);

            /*
            SigFun* sigfun = (SigFun*)df->sig;
            if (inner_ret_sigs.count == 0 && !sig_is_type(sigfun->ret, VAL_NIL)) {
                add_error(compiler, df->name, "Return type must match signature in function declaration.");
            }

            for (int i = 0; i < inner_ret_sigs.count; i++) {
                if (!same_sig(sigfun->ret, inner_ret_sigs.sigs[i])) {
                    add_error(compiler, df->name, "Return type must match signature in function declaration.");
                }
            }

            free_sig_list(&inner_ret_sigs);*/

            struct ObjFunction* f = make_function(chunk, arity);
            int f_idx = add_constant(compiler, to_function(f));
            struct ObjString* s = make_string(df->name.start, df->name.length);
            int s_idx = add_constant(compiler, to_string(s));
            emit_byte(compiler, OP_SET_DEF);
            emit_byte(compiler, s_idx);
            emit_byte(compiler, f_idx);

            return make_prim_sig(VAL_NIL);
        }
        case NODE_DECL_CLASS: {
            DeclClass* dc = (DeclClass*)node;
            add_local(compiler, dc->name, dc->sig);

            Compiler class_comp;
            Chunk chunk;
            init_chunk(&chunk);
            init_compiler(&class_comp, &chunk);
            class_comp.enclosing = (struct Compiler*)compiler;
            set_local(&class_comp, dc->name, dc->sig, 0);

            SigList ret_sigs = compile_function(&class_comp, &dc->decls);
            if (ret_sigs.count > 0) {
                add_error(compiler, dc->name, "Cannot have return statement '->' inside class definition.");
            }
            free_sig_list(&ret_sigs);

            struct ObjClass* klass = make_class(chunk);
            emit_bytes(compiler, OP_CLASS, add_constant(compiler, to_class(klass)));
            return make_prim_sig(VAL_NIL);
        }
        //statements
        case NODE_PRINT:    return compile_print(compiler, node);
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            start_scope(compiler);
            for (int i = 0; i < block->decl_list.count; i++) {
                Sig* sig = compile_node(compiler, block->decl_list.nodes[i], ret_sigs);
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

            return find_sig(compiler, gv->name);
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            Sig* right_sig = compile_node(compiler, sv->right, ret_sigs);
            Sig* var_sig = find_sig(compiler, sv->name);

            if (!same_sig(right_sig, var_sig)) {
                add_error(compiler, sv->name, "Right side type must match variable type.");
            }

            free_sig(var_sig);

            emit_byte(compiler, OP_SET_VAR);
            emit_byte(compiler, find_local(compiler, sv->name));
            if (sv->decl) {
                emit_byte(compiler, OP_POP);
            }
            return right_sig;
        }
        case NODE_CALL: {
            Call* call = (Call*)node;

            emit_byte(compiler, OP_GET_DEF);
            struct ObjString* fun_name = make_string(call->name.start, call->name.length);
            emit_byte(compiler, add_constant(compiler, to_string(fun_name)));

            //check types TODO: this won't work with new defs table
            /*
            uint8_t idx = find_local(compiler, call->name);//?
            Sig* sig = compiler->locals[idx].sig; //?
            SigList* params = &((SigFun*)sig)->params;
            if (params->count != call->arguments.count) {
                add_error(compiler, call->name, "Argument count must match declaration.");
            }

            int min = call->arguments.count < params->count ? call->arguments.count : params->count;
            for (int i = 0; i < min; i++) {
                Sig* arg_sig = compile_node(compiler, call->arguments.nodes[i], ret_sigs); //TODO: ret_sigs isn't needed here, right???
                if (!same_sig(arg_sig, params->sigs[i])) {
                    add_error(compiler, call->name, "Argument type must match parameter type.");
                }
                free_sig(arg_sig);
            }*/

            //make a new callframe
            emit_byte(compiler, OP_CALL);
            emit_byte(compiler, (uint8_t)(call->arguments.count));
            //return copy_sig(((SigFun*)sig)->ret); //TODO: need to redo this
            return make_prim_sig(VAL_NIL);
        }
        case NODE_CASCADE_CALL: {
            CascadeCall* cc = (CascadeCall*)node;
            Sig* fun_sig = compile_node(compiler, cc->function, NULL); //TODO: ret_sigs not needed here (similar to CALL), right?

            //TODO: most of this is a repeat of code in CALL - combine it
            SigList* params = &((SigFun*)fun_sig)->params;
            if (params->count != cc->arguments.count) {
                add_error(compiler, cc->name, "Argument count must match declaration.");
            }

            int min = cc->arguments.count < params->count ? cc->arguments.count : params->count;
            for (int i = 0; i < min; i++) {
                Sig* arg_sig = compile_node(compiler, cc->arguments.nodes[i], ret_sigs); //TODO: ret_sigs isn't needed here, right???
                if (!same_sig(arg_sig, params->sigs[i])) {
                    add_error(compiler, cc->name, "Argument type must match parameter type.");
                }
                free_sig(arg_sig);
            }

            //make a new callframe
            emit_byte(compiler, OP_CALL);
            emit_byte(compiler, (uint8_t)(cc->arguments.count));

            Sig* copy = copy_sig(((SigFun*)fun_sig)->ret);

            free_sig(fun_sig);

            return copy;
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

