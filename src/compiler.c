#include <stdio.h>
#include <string.h>
#include <time.h>

#include "compiler.h"
#include "memory.h"
#include "obj_string.h"
#include "obj_function.h"
#include "obj_class.h"

struct Compiler* current_compiler = NULL;
struct Compiler* class_compiler = NULL;
struct Sig* compile_node(struct Compiler* compiler, struct Node* node, struct SigList* ret_sigs);
static struct SigList* compile_function(struct Compiler* compiler, NodeList* nl);

static void add_error(struct Compiler* compiler, Token token, const char* message) {
    CompileError error;
    error.token = token;
    error.message = message;
    compiler->errors[compiler->error_count] = error;
    compiler->error_count++;
}

static void copy_errors(struct Compiler* dest, struct Compiler* src) {
    for (int i = 0; i < src->error_count; i++) {
        dest->errors[dest->error_count] = src->errors[i];
        dest->error_count++;
    }
}

static void grow_capacity(struct Compiler* compiler) {
    int new_capacity = compiler->function->chunk.capacity == 0 ? 8 : compiler->function->chunk.capacity * 2;
    compiler->function->chunk.codes = GROW_ARRAY(compiler->function->chunk.codes, OpCode, new_capacity, compiler->function->chunk.capacity);
    compiler->function->chunk.capacity = new_capacity;
}

static int add_constant(struct Compiler* compiler, Value value) {
    return add_value(&compiler->function->chunk.constants, value);
}

static void emit_byte(struct Compiler* compiler, uint8_t byte) {
    if (compiler->function->chunk.count + 1 > compiler->function->chunk.capacity) {
        grow_capacity(compiler);
    }

    compiler->function->chunk.codes[compiler->function->chunk.count] = byte;
    compiler->function->chunk.count++;
}

static void emit_short(struct Compiler* compiler, uint16_t bytes) {
    if (compiler->function->chunk.count + 2 > compiler->function->chunk.capacity) {
        grow_capacity(compiler);
    }

    memcpy(&compiler->function->chunk.codes[compiler->function->chunk.count], &bytes, sizeof(uint16_t));
    compiler->function->chunk.count += 2;
}

static void emit_bytes(struct Compiler* compiler, uint8_t byte1, uint8_t byte2) {
    emit_byte(compiler, byte1);
    emit_byte(compiler, byte2);
}

static int emit_jump(struct Compiler* compiler, OpCode op) {
    emit_byte(compiler, op);
    emit_byte(compiler, 0xff);
    emit_byte(compiler, 0xff);
    return compiler->function->chunk.count;
}

static void patch_jump(struct Compiler* compiler, int index) {
    uint16_t destination = (uint16_t)(compiler->function->chunk.count - index);
    memcpy(&compiler->function->chunk.codes[index - 2], &destination, sizeof(uint16_t));
}

static void emit_jump_by(struct Compiler* compiler, OpCode op, int index) {
    emit_byte(compiler, op);
    emit_short(compiler, (uint16_t)index);
}


static struct Sig* compile_literal(struct Compiler* compiler, struct Node* node) {
    Literal* literal = (Literal*)node;
    switch(literal->name.type) {
        case TOKEN_INT: {
            int32_t integer = (int32_t)strtol(literal->name.start, NULL, 10);
            emit_byte(compiler, OP_CONSTANT); 
            emit_short(compiler, add_constant(compiler, to_integer(integer)));
            return make_prim_sig(VAL_INT);
        }
        case TOKEN_FLOAT: {
            double f = strtod(literal->name.start, NULL);
            emit_byte(compiler, OP_CONSTANT);
            emit_short(compiler, add_constant(compiler, to_float(f)));
            return make_prim_sig(VAL_FLOAT);
        }
        case TOKEN_STRING: {
            struct ObjString* str = make_string(literal->name.start, literal->name.length);
            push_root(to_string(str));
            emit_byte(compiler, OP_CONSTANT);
            emit_short(compiler, add_constant(compiler, to_string(str)));
            pop_root();
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

static struct Sig* compile_unary(struct Compiler* compiler, struct Node* node) {
    Unary* unary = (Unary*)node;
    struct Sig* sig = compile_node(compiler, unary->right, NULL);
    emit_byte(compiler, OP_NEGATE);
    return sig;
}

static struct Sig* compile_logical(struct Compiler* compiler, struct Node* node) {
    Logical* logical = (Logical*)node;

    if (logical->name.type == TOKEN_AND) {
        struct Sig* left_type = compile_node(compiler, logical->left, NULL);
        int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
        struct Sig* right_type = compile_node(compiler, logical->right, NULL);
        patch_jump(compiler, false_jump);
        if (!same_sig(left_type, right_type)) {
            add_error(compiler, logical->name, "Left and right types must match.");
        }
        return make_prim_sig(VAL_BOOL);
    }

    if (logical->name.type == TOKEN_OR) {
        struct Sig* left_type = compile_node(compiler, logical->left, NULL);
        int true_jump = emit_jump(compiler, OP_JUMP_IF_TRUE);
        emit_byte(compiler, OP_POP);
        struct Sig* right_type = compile_node(compiler, logical->right, NULL);
        patch_jump(compiler, true_jump);
        if (!same_sig(left_type, right_type)) {
            add_error(compiler, logical->name, "Left and right types must match.");
        }
        return make_prim_sig(VAL_BOOL);
    }


    struct Sig* left_type = compile_node(compiler, logical->left, NULL);
    struct Sig* right_type = compile_node(compiler, logical->right, NULL);
    if (!same_sig(left_type, right_type)) {
        add_error(compiler, logical->name, "Left and right types must match.");
    }
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

static struct Sig* compile_binary(struct Compiler* compiler, struct Node* node) {
    Binary* binary = (Binary*)node;

    struct Sig* type1 = compile_node(compiler, binary->left, NULL);
    struct Sig* type2 = compile_node(compiler, binary->right, NULL);
    if (!same_sig(type1, type2)) {
        add_error(compiler, binary->name, "Left and right types must match.");
    }
    switch(binary->name.type) {
        case TOKEN_PLUS: emit_byte(compiler, OP_ADD); break;
        case TOKEN_MINUS: emit_byte(compiler, OP_SUBTRACT); break;
        case TOKEN_STAR: emit_byte(compiler, OP_MULTIPLY); break;
        case TOKEN_SLASH: emit_byte(compiler, OP_DIVIDE); break;
        case TOKEN_MOD: emit_byte(compiler, OP_MOD); break;
    }

    return type1;
}

static void set_local(struct Compiler* compiler, Token name, struct Sig* sig, int index) {
    Local local;
    local.name = name;
    local.sig = sig;
    local.depth = compiler->scope_depth;
    local.is_captured = false;
    compiler->locals[index] = local;
}

static void add_local(struct Compiler* compiler, Token name, struct Sig* sig) {
    set_local(compiler, name, sig, compiler->locals_count);
    compiler->locals_count++;
}


static int add_upvalue(struct Compiler* compiler, struct Upvalue upvalue) {
    for (int i = 0; i < compiler->upvalue_count; i++) {
        struct Upvalue* cached = &compiler->upvalues[i];
        if (upvalue.index == cached->index && upvalue.is_local == cached->is_local) {
            return i;
        }
    }

    compiler->upvalues[compiler->upvalue_count] = upvalue;
    return compiler->upvalue_count++;
}

static struct Sig* resolve_sig(struct Compiler* compiler, Token name) {
    struct Compiler* current = compiler;
    do {
        for (int i = current->locals_count - 1; i >= 0; i--) {
            Local* local = &current->locals[i];
            if (local->name.length == name.length && memcmp(local->name.start, name.start, name.length) == 0) {
                return current->locals[i].sig;
            }
        }
        current = current->enclosing;
    } while(current != NULL);

    add_error(compiler, name, "Local variable not declared.");

    return NULL;
}

static int resolve_local(struct Compiler* compiler, Token name) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->name.length == name.length && 
            memcmp(local->name.start, name.start, name.length) == 0) {

            return i;
        }
    }

    return -1;
}

static int resolve_upvalue(struct Compiler* compiler, Token name) {
    if (compiler->enclosing == NULL) return -1;

    int local_index = resolve_local(compiler->enclosing, name);
    if (local_index != -1) {
        compiler->enclosing->locals[local_index].is_captured = true;
        struct Upvalue upvalue;
        upvalue.index = local_index;
        upvalue.is_local = true;
        int upvalue_idx = add_upvalue(compiler, upvalue);
        return upvalue_idx;
    }

    int recursive_idx = resolve_upvalue(compiler->enclosing, name);
    if (recursive_idx != -1) {
        struct Upvalue upvalue;
        upvalue.index = recursive_idx;
        upvalue.is_local = false;
        int upvalue_idx = add_upvalue(compiler, upvalue);
        return upvalue_idx;
    }

    return -1;
}


static void start_scope(struct Compiler* compiler) {
    compiler->scope_depth++;
}

static void end_scope(struct Compiler* compiler) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->depth < compiler->scope_depth) {
            break;
        }

        if (local->is_captured) {
            emit_byte(compiler, OP_CLOSE_UPVALUE);
        } else {
            emit_byte(compiler, OP_POP);
        }

        compiler->locals_count--;
    }

    compiler->scope_depth--;
}

static struct Sig* compile_node(struct Compiler* compiler, struct Node* node, struct SigList* ret_sigs) {
    if (node == NULL) {
        return make_prim_sig(VAL_NIL);
    }
    switch(node->type) {
        //declarations
        case NODE_DECL_VAR: {
            DeclVar* dv = (DeclVar*)node;
            add_local(compiler, dv->name, dv->sig);

            struct Sig* sig = compile_node(compiler, dv->right, NULL);
            if (sig->type == SIG_IDENTIFIER) {
                sig = resolve_sig(compiler, ((struct SigIdentifier*)sig)->identifier);
            }

            struct Sig* decl_sig = dv->sig;
            if (decl_sig->type == SIG_IDENTIFIER) {
                decl_sig = resolve_sig(compiler, ((struct SigIdentifier*)decl_sig)->identifier);
            }

            if (!sig_is_type(sig, VAL_NIL) && !same_sig(sig, decl_sig)) {
                add_error(compiler, dv->name, "Declaration type and right hand side type must match.");
            }
            return make_prim_sig(VAL_NIL);
        }
        case NODE_DECL_FUN: {
            DeclFun* df = (DeclFun*)node;

            struct Compiler func_comp;
            init_compiler(&func_comp, df->name.start, df->name.length, df->parameters.count);

            set_local(&func_comp, df->name, df->sig, 0);
            add_node(&df->parameters, df->body);
            struct SigList* inner_ret_sigs = compile_function(&func_comp, &df->parameters); 

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(func_comp.function);
#endif

            copy_errors(compiler, &func_comp);

            struct SigFun* sigfun = (struct SigFun*)df->sig;
            if (inner_ret_sigs->count == 0 && !sig_is_type(sigfun->ret, VAL_NIL)) {
                add_error(compiler, df->name, "Return type must match signature in function declaration.");
            }

            struct Sig* ret_sig = sigfun->ret;
            if (ret_sig->type == SIG_IDENTIFIER) {
                ret_sig = resolve_sig(compiler, ((struct SigIdentifier*)ret_sig)->identifier);
            }
            
            for (int i = 0; i < inner_ret_sigs->count; i++) {
                struct Sig* inner_sig = inner_ret_sigs->sigs[i];
                if (inner_sig->type == SIG_IDENTIFIER) {
                    inner_sig = resolve_sig(compiler, ((struct SigIdentifier*)inner_sig)->identifier);
                }
                if (!same_sig(ret_sig, inner_sig)) {
                    add_error(compiler, df->name, "Return type must match signature in function declaration.");
                }
            }

            int f_idx = add_constant(compiler, to_function(func_comp.function));
            emit_byte(compiler, OP_FUN);
            emit_short(compiler, f_idx);
            emit_byte(compiler, func_comp.upvalue_count);
            for (int i = 0; i < func_comp.upvalue_count; i++) {
                emit_byte(compiler, func_comp.upvalues[i].is_local);
                emit_byte(compiler, func_comp.upvalues[i].index);
            }

            free_compiler(&func_comp);

            return df->sig;
        }
        case NODE_DECL_CLASS: {
            DeclClass* dc = (DeclClass*)node;

            struct ObjString* name = make_string(dc->name.start, dc->name.length);
            push_root(to_string(name));
            struct ObjClass* klass = make_class(name);
            push_root(to_class(klass));

            emit_byte(compiler, OP_CLASS);
            emit_short(compiler, add_constant(compiler, to_class(klass)));

            for (int i = 0; i < dc->decls.count; i++) {
                struct Node* node = dc->decls.nodes[i];
                DeclVar* dv = (DeclVar*)node;
                struct ObjString* prop_name = make_string(dv->name.start, dv->name.length);

                push_root(to_string(prop_name));

                struct Sig* class_ret_sigs = make_list_sig();
                struct Sig* sig = compile_node(compiler, node, (struct SigList*)class_ret_sigs);
                emit_byte(compiler, OP_ADD_PROP);
                emit_short(compiler, add_constant(compiler, to_string(prop_name)));
                compiler->locals_count--;
                pop_root();
            }

            pop_root();
            pop_root();

            return make_prim_sig(VAL_NIL);
        }
        case NODE_INST_CLASS: {
            InstClass* ic = (InstClass*)node;

            struct Sig* klass_sig = compile_node(compiler, ic->klass, ret_sigs);
            struct Sig* decl_sig = resolve_sig(compiler, ic->klass_type);
            if (!is_duck((struct SigClass*)decl_sig, (struct SigClass*)klass_sig)) {
                add_error(compiler, ic->name, "Declaration type and class constructor type must match.");
            }
            emit_byte(compiler, OP_INSTANCE);
            add_local(compiler, ic->name, klass_sig);
            return make_prim_sig(VAL_NIL);
        }
        //statements
        case NODE_EXPR_STMT: {
            ExprStmt* es = (ExprStmt*)node;
            struct Sig* sig = compile_node(compiler, es->expr, ret_sigs);
            if (!sig_is_type(sig, VAL_NIL)) {
                emit_byte(compiler, OP_POP);
            }
            return make_prim_sig(VAL_NIL);
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            start_scope(compiler);
            for (int i = 0; i < block->decl_list.count; i++) {
                struct Sig* sig = compile_node(compiler, block->decl_list.nodes[i], ret_sigs);
            }
            end_scope(compiler);
            return make_prim_sig(VAL_NIL);
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            struct Sig* cond = compile_node(compiler, ie->condition, ret_sigs);
            if (!sig_is_type(cond, VAL_BOOL)) {
                add_error(compiler, ie->name, "Condition must evaluate to boolean.");
            }

            int jump_then = emit_jump(compiler, OP_JUMP_IF_FALSE); 

            emit_byte(compiler, OP_POP);
            struct Sig* then_sig = compile_node(compiler, ie->then_block, ret_sigs);
            int jump_else = emit_jump(compiler, OP_JUMP);

            patch_jump(compiler, jump_then);
            emit_byte(compiler, OP_POP);
            struct Sig* else_sig = compile_node(compiler, ie->else_block, ret_sigs);
            patch_jump(compiler, jump_else);
            return make_prim_sig(VAL_NIL);
        }
        case NODE_WHILE: {
            While* wh = (While*)node;
            int start = compiler->function->chunk.count;
            struct Sig* cond = compile_node(compiler, wh->condition, ret_sigs);
            if (!sig_is_type(cond, VAL_BOOL)) {
                add_error(compiler, wh->name, "Condition must evaluate to boolean.");
            }

            int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);

            emit_byte(compiler, OP_POP);
            struct Sig* then_block = compile_node(compiler, wh->then_block, ret_sigs);

            //+3 to include OP_JUMP_BACK and uint16_t for jump distance
            int from = compiler->function->chunk.count + 3;
            emit_jump_by(compiler, OP_JUMP_BACK, from - start);
            patch_jump(compiler, false_jump);   
            emit_byte(compiler, OP_POP);
            return make_prim_sig(VAL_NIL);
        }
        case NODE_FOR: {
            For* fo = (For*)node;
            struct Sig* init = compile_node(compiler, fo->initializer, ret_sigs); //should leave no value on stack

            int condition_start = compiler->function->chunk.count;
            struct Sig* cond = compile_node(compiler, fo->condition, ret_sigs);
            if (!sig_is_type(cond, VAL_BOOL)) {
                add_error(compiler, fo->name, "Condition must evaluate to boolean.");
            }

            int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
            int body_jump = emit_jump(compiler, OP_JUMP);

            int update_start = compiler->function->chunk.count;
            if (fo->update) {
                struct Sig* up = compile_node(compiler, fo->update, ret_sigs);
                emit_byte(compiler, OP_POP); //pop update
            }
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->function->chunk.count + 3 - condition_start);

            patch_jump(compiler, body_jump);
            emit_byte(compiler, OP_POP); //pop condition if true
            struct Sig* then_sig = compile_node(compiler, fo->then_block, ret_sigs);
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->function->chunk.count + 3 - update_start);

            patch_jump(compiler, exit_jump);
            emit_byte(compiler, OP_POP); //pop condition if false
            return make_prim_sig(VAL_NIL);
        }
        case NODE_RETURN: {
            Return* ret = (Return*)node;
            struct Sig* sig = compile_node(compiler, ret->right, ret_sigs);
            if (sig_is_type(sig, VAL_NIL)) {
                emit_byte(compiler, OP_NIL);
            }
            emit_byte(compiler, OP_RETURN);
            add_sig(ret_sigs, sig);
            return sig;
        }
        //expressions
        case NODE_LITERAL:      return compile_literal(compiler, node);
        case NODE_BINARY:       return compile_binary(compiler, node);
        case NODE_LOGICAL:      return compile_logical(compiler, node);
        case NODE_UNARY:        return compile_unary(compiler, node);
        case NODE_GET_PROP: {
            GetProp* gp = (GetProp*)node;
            struct Sig* sig_inst = compile_node(compiler, gp->inst, ret_sigs);
            if (sig_inst == NULL) return make_prim_sig(VAL_NIL);

            if (sig_inst->type == SIG_IDENTIFIER) {
                sig_inst = resolve_sig(compiler, ((struct SigIdentifier*)sig_inst)->identifier);
            }

            emit_byte(compiler, OP_GET_PROP);
            struct ObjString* name = make_string(gp->prop.start, gp->prop.length);
            push_root(to_string(name));
            emit_short(compiler, add_constant(compiler, to_string(name))); 
            pop_root();

            Value sig_val = to_nil();
            get_from_table(&((struct SigClass*)sig_inst)->props, name, &sig_val);

            if (IS_NIL(sig_val)) {
                add_error(compiler, gp->prop, "Property doesn't exist on instance");
                return make_prim_sig(VAL_NIL);
            }

            return sig_val.as.sig_type;
        }
        case NODE_SET_PROP: {
            SetProp* sp = (SetProp*)node;
            struct Sig* right_sig = compile_node(compiler, sp->right, ret_sigs);
            struct Sig* sig_inst = compile_node(compiler, sp->inst, ret_sigs);
            if (sig_inst->type == SIG_IDENTIFIER) {
                sig_inst = resolve_sig(compiler, ((struct SigIdentifier*)sig_inst)->identifier);
            }

            struct ObjString* name = make_string(sp->prop.start, sp->prop.length);
            push_root(to_string(name));
            emit_byte(compiler, OP_SET_PROP);
            emit_short(compiler, add_constant(compiler, to_string(name))); 
            pop_root();

            Value sig_val = to_nil();
            get_from_table(&((struct SigClass*)sig_inst)->props, name, &sig_val);

            if (!same_sig(sig_val.as.sig_type, right_sig)) {
                add_error(compiler, sp->prop, "Property and assignment types must match.");
                return make_prim_sig(VAL_NIL);
            }

            return right_sig;

        }
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;

            int idx = resolve_local(compiler, gv->name);
            if (idx != -1) {
                emit_byte(compiler, OP_GET_LOCAL);
                emit_byte(compiler, idx);
                return resolve_sig(compiler, gv->name);
            }

            int upvalue_idx = resolve_upvalue(compiler, gv->name);
            if (upvalue_idx != -1) {
                emit_byte(compiler, OP_GET_UPVALUE);
                emit_byte(compiler, upvalue_idx);
                return resolve_sig(compiler, gv->name);
            }

            add_error(compiler, gv->name, "Local variable not declared.");

            return NULL;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            struct Sig* right_sig = compile_node(compiler, sv->right, ret_sigs);
            if (right_sig->type == SIG_IDENTIFIER) {
                right_sig = resolve_sig(compiler, ((struct SigIdentifier*)right_sig)->identifier);
            }

            Token var = ((GetVar*)(sv->left))->name;
            struct Sig* var_sig = resolve_sig(compiler, var);
            int idx = resolve_local(compiler, var);
            if (idx != -1) {
                if (!same_sig(var_sig, right_sig)) {
                    add_error(compiler, var, "Right side type must match variable type.");
                }

                emit_byte(compiler, OP_SET_LOCAL);
                emit_byte(compiler, idx);
                return right_sig;
            }

            int upvalue_idx = resolve_upvalue(compiler, var);
            if (upvalue_idx != -1) {
                if (!same_sig(var_sig, right_sig)) {
                    add_error(compiler, var, "Right side type must match variable type.");
                }

                emit_byte(compiler, OP_SET_UPVALUE);
                emit_byte(compiler, upvalue_idx);
                return right_sig;
            }

            add_error(compiler, var, "Local variable not declared.");
            return make_prim_sig(VAL_NIL);
        }
        case NODE_CALL: {
            Call* call = (Call*)node;

            struct Sig* sig = compile_node(compiler, call->left, ret_sigs);

            if (sig->type == SIG_CLASS) {
                struct SigClass* sig_class = (struct SigClass*)sig;
                emit_byte(compiler, OP_INSTANCE);
                return sig;
            }

            if (sig->type != SIG_FUN) {
                add_error(compiler, call->name, "Calls must be used on a function type.");
                return make_prim_sig(VAL_NIL);
            }

            struct SigFun* sig_fun = (struct SigFun*)sig;
            struct SigList* params = (struct SigList*)(sig_fun->params);

            if (call->arguments.count != params->count) {
                add_error(compiler, call->name, "Argument count must match function parameter count.");
                return make_prim_sig(VAL_NIL);
            }

            int min = call->arguments.count < params->count ? call->arguments.count : params->count; //Why is this needed?
            for (int i = 0; i < min; i++) {
                struct Sig* arg_sig = compile_node(compiler, call->arguments.nodes[i], ret_sigs);
                if (arg_sig->type == SIG_IDENTIFIER) {
                    arg_sig = resolve_sig(compiler, ((struct SigIdentifier*)arg_sig)->identifier);
                }

                struct Sig* param_sig = params->sigs[i];
                if (param_sig->type == SIG_IDENTIFIER) {
                    param_sig = resolve_sig(compiler, ((struct SigIdentifier*)param_sig)->identifier);
                }

                if (!same_sig(arg_sig, param_sig)) {
                    add_error(compiler, call->name, "Argument type must match parameter type.");
                }
            }

            emit_byte(compiler, OP_CALL);
            emit_byte(compiler, (uint8_t)(call->arguments.count));

            return sig_fun->ret;
        }
    } 
}

void init_compiler(struct Compiler* compiler, const char* start, int length, int parameter_count) {
    struct ObjString* name = make_string(start, length);
    push_root(to_string(name));
    compiler->scope_depth = 0;
    compiler->locals_count = 1; //first slot is for function def
    compiler->error_count = 0;
    compiler->function = make_function(name, parameter_count);
    push_root(to_function(compiler->function));
    compiler->enclosing = NULL;
    compiler->upvalue_count = 0;
    compiler->signatures = NULL;

    compiler->enclosing = current_compiler;
    current_compiler = compiler;
}

void free_compiler(struct Compiler* compiler) {
    while (compiler->signatures != NULL) {
        struct Sig* previous = compiler->signatures;
        compiler->signatures = compiler->signatures->next;
        free_sig(previous);
    }
    current_compiler = compiler->enclosing;
    pop_root();
    pop_root();
}

static struct SigList* compile_function(struct Compiler* compiler, NodeList* nl) {
    struct Sig* ret_sigs = make_list_sig();
    for (int i = 0; i < nl->count; i++) {
        struct Sig* sig = compile_node(compiler, nl->nodes[i], (struct SigList*)ret_sigs);
    }

    if (((struct SigList*)ret_sigs)->count == 0) {
        emit_byte(compiler, OP_NIL);
        emit_byte(compiler, OP_RETURN);
    }
    return (struct SigList*)ret_sigs;
}

static Value clock_native(int arg_count, Value* args) {
    return to_float((double)clock() / CLOCKS_PER_SEC);
}

static Value print_native(int arg_count, Value* args) {
    Value value = args[0];
    switch(value.type) {
        case VAL_STRING:
            struct ObjString* str = value.as.string_type;
            printf("%.*s\n", str->length, str->chars);
            break;
        case VAL_INT:
            printf("%d\n", value.as.integer_type);
           break;
        case VAL_FLOAT:
            printf("%f\n", value.as.float_type);
          break; 
    }
    return to_nil();
}

static struct Sig* define_native(struct Compiler* compiler, const char* name, Value (*function)(int, Value*), struct Sig* sig) {
    add_local(compiler, make_artificial_token(name), sig);
    emit_byte(compiler, OP_NATIVE);
    Value native = to_native(make_native(function));
    push_root(native);
    emit_short(compiler, add_constant(compiler, native));
    pop_root();
}

ResultCode compile_script(struct Compiler* compiler, NodeList* nl) {
    define_native(compiler, "clock", clock_native, make_fun_sig(make_list_sig(), make_prim_sig(VAL_INT)));
    struct Sig* sl = make_list_sig();
    struct Sig* str_sig = make_prim_sig(VAL_STRING);
    struct Sig* int_sig = make_prim_sig(VAL_INT);
    struct Sig* float_sig = make_prim_sig(VAL_FLOAT);
    str_sig->opt = int_sig;
    int_sig->opt = float_sig;
    add_sig((struct SigList*)sl, str_sig);
    define_native(compiler, "print", print_native, make_fun_sig((struct Sig*)sl, make_prim_sig(VAL_NIL)));

    struct SigList* ret_sigs = compile_function(compiler, nl);

    if (compiler->error_count > 0) {
        for (int i = 0; i < compiler->error_count; i++) {
            printf("[line %d] %s\n", compiler->errors[i].token.line, compiler->errors[i].message);
        }
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}


void print_locals(struct Compiler* compiler) {
    for (int i = 0; i < compiler->locals_count; i ++) {
        Local* local = &compiler->locals[i];
        printf("%.*s\n", local->name.length, local->name.start);
    }
}

