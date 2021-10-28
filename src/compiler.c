#include <stdio.h>
#include <string.h>
#include <math.h>

#include "compiler.h"
#include "memory.h"
#include "obj.h"

#define MAX_IS_STATEMENTS 256
#define MAX_INFERRED_SEQ_VARS 256

#define COMPILE_NODE_OLD(node, node_type) \
            if (compile_node(compiler, node, node_type) == RESULT_FAILED) return RESULT_FAILED

#define CHECK_TYPE(fail_condition, token, msg) \
                if (fail_condition) { \
                    add_error(compiler, token, msg); \
                    return RESULT_FAILED; \
                }

#define COMPILE_NODE(node, node_type) \
            if (compile_node(compiler, node, node_type) == RESULT_FAILED) result = RESULT_FAILED

#define EMIT_ERROR_IF(error_condition, token, msg) \
            if (error_condition) { \
                add_error(compiler, token, msg); \
                result = RESULT_FAILED; \
            }

struct Compiler* current_compiler = NULL;
struct Compiler* script_compiler = NULL;
static ResultCode compile_node(struct Compiler* compiler, struct Node* node, struct Type** node_type);
static ResultCode compile_function(struct Compiler* compiler, struct NodeList* nl, struct TypeArray** type_array);

static bool struct_or_function_to_nil(struct Type* var_type, struct Type* right_type) {
    if (right_type->type != TYPE_NIL) return false;
    if (var_type->type != TYPE_FUN && var_type->type != TYPE_STRUCT) return false;
    return true;
}

static void add_error(struct Compiler* compiler, Token token, const char* message) {
    struct Error error;
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


static ResultCode compile_literal(struct Compiler* compiler, struct Node* node, struct Type** node_type) {
    Literal* literal = (Literal*)node;
    switch(literal->name.type) {
        case TOKEN_INT: {
            int32_t integer = (int32_t)strtol(literal->name.start, NULL, 10);
            emit_byte(compiler, OP_CONSTANT); 
            emit_short(compiler, add_constant(compiler, to_integer(integer)));
            *node_type = make_int_type();
            return RESULT_SUCCESS;
        }
        case TOKEN_FLOAT: {
            double f = strtod(literal->name.start, NULL);
            emit_byte(compiler, OP_CONSTANT);
            emit_short(compiler, add_constant(compiler, to_float(f)));
            *node_type = make_float_type();
            return RESULT_SUCCESS;
        }
        case TOKEN_STRING: {
            struct ObjString* str = make_string(literal->name.start, literal->name.length);
            push_root(to_string(str));
            emit_byte(compiler, OP_CONSTANT);
            emit_short(compiler, add_constant(compiler, to_string(str)));
            pop_root();
            *node_type = make_string_type();
            return RESULT_SUCCESS;
        }
        case TOKEN_TRUE: {
            emit_byte(compiler, OP_TRUE);
            *node_type = make_bool_type();
            return RESULT_SUCCESS;
        }
        case TOKEN_FALSE: {
            emit_byte(compiler, OP_FALSE);
            *node_type = make_bool_type();
            return RESULT_SUCCESS;
        }
    }
    add_error(compiler, literal->name, "Literal type undefined.");
    return RESULT_FAILED;
}

static ResultCode compile_unary(struct Compiler* compiler, struct Node* node, struct Type** node_type) {
    Unary* unary = (Unary*)node;
    struct Type* type;
    COMPILE_NODE_OLD(unary->right, &type);
    emit_byte(compiler, OP_NEGATE);
    *node_type = type;
    return RESULT_SUCCESS;
}

static ResultCode compile_logical(struct Compiler* compiler, struct Node* node, struct Type** node_type) {
    Logical* logical = (Logical*)node;

    if (logical->name.type == TOKEN_AND) {
        struct Type* left_type;
        COMPILE_NODE_OLD(logical->left, &left_type);
        int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
        struct Type* right_type;
        COMPILE_NODE_OLD(logical->right, &right_type);
        patch_jump(compiler, false_jump);
        CHECK_TYPE(!same_type(left_type, right_type), logical->name, "Left and right types must match.");
        *node_type = make_bool_type();
        return RESULT_SUCCESS;
    }

    if (logical->name.type == TOKEN_OR) {
        struct Type* left_type;
        COMPILE_NODE_OLD(logical->left, &left_type);
        int true_jump = emit_jump(compiler, OP_JUMP_IF_TRUE);
        emit_byte(compiler, OP_POP);
        struct Type* right_type;
        COMPILE_NODE_OLD(logical->right, &right_type);
        patch_jump(compiler, true_jump);
        CHECK_TYPE(!same_type(left_type, right_type), logical->name, "Left and right types must match.");
        *node_type = make_bool_type();
        return RESULT_SUCCESS;
    }

    if (logical->name.type == TOKEN_IN) {
        struct Type* element_type;
        COMPILE_NODE_OLD(logical->left, &element_type);
        struct Type* list_type;
        COMPILE_NODE_OLD(logical->right, &list_type);

        CHECK_TYPE(list_type->type != TYPE_LIST, logical->name, "Identifier after 'in' must reference a List.");

        CHECK_TYPE(!same_type(element_type, ((struct TypeList*)list_type)->type), logical->name, "Type left of 'in' must match List element type.");

        emit_byte(compiler, OP_IN_LIST);
        *node_type = make_bool_type();
        return RESULT_SUCCESS;
    }

    struct Type* left_type;
    COMPILE_NODE_OLD(logical->left, &left_type);
    struct Type* right_type;
    COMPILE_NODE_OLD(logical->right, &right_type);

    CHECK_TYPE(!same_type(left_type, right_type) && !struct_or_function_to_nil(left_type, right_type), logical->name, "Left and right types must match.");
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
    *node_type = make_bool_type();
    return RESULT_SUCCESS;
}

static ResultCode compile_binary(struct Compiler* compiler, struct Node* node, struct Type** node_type) {
    Binary* binary = (Binary*)node;

    struct Type* type1;
    COMPILE_NODE_OLD(binary->left, &type1);
    struct Type* type2;
    COMPILE_NODE_OLD(binary->right, &type2);
    CHECK_TYPE(!same_type(type1, type2), binary->name, "Left and right types must match.");
    switch(binary->name.type) {
        case TOKEN_PLUS: {
            if (type1->type != TYPE_INT && type1->type != TYPE_FLOAT && type1->type != TYPE_STRING) {
                add_error(compiler, binary->name, "'+' can only be used on ints, floats and strings");
                return RESULT_FAILED;
            }
            emit_byte(compiler, OP_ADD);
            break;
        }
        case TOKEN_MINUS: emit_byte(compiler, OP_SUBTRACT); break;
        case TOKEN_STAR: emit_byte(compiler, OP_MULTIPLY); break;
        case TOKEN_SLASH: emit_byte(compiler, OP_DIVIDE); break;
        case TOKEN_MOD: emit_byte(compiler, OP_MOD); break;
    }

    *node_type = type1;
    return RESULT_SUCCESS;
}

static void set_local(struct Compiler* compiler, Token name, struct Type* type, int index) {
    Local local;
    local.name = name;
    local.type = type;
    local.depth = compiler->scope_depth;
    local.is_captured = false;
    compiler->locals[index] = local;
}

static int add_local(struct Compiler* compiler, Token name, struct Type* type) {
    set_local(compiler, name, type, compiler->locals_count);
    compiler->locals_count++;
    return compiler->locals_count - 1;
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

static struct Type* resolve_type(struct Compiler* compiler, Token name) {
    struct Compiler* current = compiler;
    do {
        //check locals
        for (int i = current->locals_count - 1; i >= 0; i--) {
            Local* local = &current->locals[i];
            if (same_token_literal(local->name, name)) {
                return current->locals[i].type;
            }
        }
        current = current->enclosing;
    } while(current != NULL);

    //check globals
    struct ObjString* str = make_string(name.start, name.length);
    Value val;
    if (get_entry(&script_compiler->globals, str, &val)) {
        return val.as.type_type;
    }

    add_error(compiler, name, "Local variable not declared.");

    return NULL;
}

static bool declared_in_scope(struct Compiler* compiler, Token name) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (same_token_literal(local->name, name)) {
            if (compiler->scope_depth == local->depth) return true; 
            else return false;
        }
    }

    return false;
}

static int resolve_local(struct Compiler* compiler, Token name) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (same_token_literal(local->name, name)) {
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

static ResultCode compile_decl_var_and_get_type(struct Compiler* compiler, DeclVar* dv, struct Type** node_type) {
    struct Type* type = NULL;

    if (declared_in_scope(compiler, dv->name)) {
        add_error(compiler, dv->name, "Identifier already defined.");
        return RESULT_FAILED;
    }

    //inferred type, defined
    if (dv->type->type == TYPE_INFER) {
        int idx = add_local(compiler, dv->name, dv->type);

        COMPILE_NODE_OLD(dv->right, &type);
        CHECK_TYPE(type->type == TYPE_NIL , dv->name,
                   "Inferred type cannot be assigned to 'nil.");

        CHECK_TYPE(type->type == TYPE_DECL, dv->name, 
                   "Variable cannot be assigned to a user declared type.");

        set_local(compiler, dv->name, type, idx);
    }
    
    //explicit type, defined and parameters
    if (dv->type->type != TYPE_INFER) {
        int idx = add_local(compiler, dv->name, dv->type);
        COMPILE_NODE_OLD(dv->right, &type);

        if (type->type != TYPE_NIL) {
            set_local(compiler, dv->name, type, idx);

            if (dv->type->type == TYPE_LIST) {
                struct TypeList* tl = (struct TypeList*)(dv->type);
                CHECK_TYPE(tl->type->type == TYPE_NIL, dv->name, "List value type cannot be 'nil'.");
            }

            if (dv->type->type == TYPE_MAP) {
                struct TypeMap* tm = (struct TypeMap*)(dv->type);
                CHECK_TYPE(tm->type->type == TYPE_NIL, dv->name, "Map value type cannot be 'nil'.");
            }

            CHECK_TYPE(!same_type(dv->type, type), dv->name,
                       "Declaration type and right hand side type must match.");
        }

    }

    *node_type = type;
    return RESULT_SUCCESS;
}

static ResultCode set_var_to_stack_idx(struct Compiler* compiler, Token var, int depth) {
    int idx = resolve_local(compiler, var);
    OpCode op = OP_SET_LOCAL;
    if (idx == -1) {
        idx = resolve_upvalue(compiler, var);
        op = OP_SET_UPVALUE;
    }

    if (idx != -1) {
        emit_byte(compiler, op);
        emit_byte(compiler, idx);
        emit_byte(compiler, depth);
        return RESULT_SUCCESS;
    }

    return RESULT_FAILED;
}

static ResultCode compile_set_element(struct Compiler* compiler, Token name, struct Type* left_type, struct Type* idx_type, 
                                      struct Type* right_type, int depth) {
    if (left_type->type != TYPE_LIST && left_type->type != TYPE_MAP) {
        add_error(compiler, name, "[] access must be used on a list or map type.");
        return RESULT_FAILED;
    }

    struct Type* template;
    if (left_type->type == TYPE_LIST) {
        CHECK_TYPE(idx_type->type != TYPE_INT, name, "Index must be integer type.");
        template = ((struct TypeList*)left_type)->type;
        CHECK_TYPE(!same_type(template, right_type), name, "List type and right side type must match.");
    }

    if (left_type->type == TYPE_MAP) {
        CHECK_TYPE(idx_type->type != TYPE_STRING, name, "Key must be string type.");
        template = ((struct TypeMap*)left_type)->type;
        CHECK_TYPE(!same_type(template, right_type), name, "Map type and right side type must match.");
    }

    emit_byte(compiler, OP_SET_ELEMENT);
    emit_byte(compiler, depth);
    return RESULT_SUCCESS;
}

static ResultCode compile_set_prop(struct Compiler* compiler, Token prop, struct Type* inst_type, struct Type* right_type, int depth) {
    if (inst_type->type == TYPE_LIST) {
        if (same_token_literal(prop, make_token(TOKEN_DUMMY, 0, "size", 4))) {
            emit_byte(compiler, OP_SET_SIZE);
            return RESULT_SUCCESS;
        }
        add_error(compiler, prop, "Property doesn't exist on Lists.");
        return RESULT_FAILED;
    }

    if (inst_type->type == TYPE_STRUCT) {
        struct ObjString* name = make_string(prop.start, prop.length);
        push_root(to_string(name));
        emit_byte(compiler, OP_SET_PROP);
        emit_short(compiler, add_constant(compiler, to_string(name))); 
        emit_byte(compiler, depth);
        pop_root();

        Value type_val = to_nil();
        get_entry(&((struct TypeStruct*)inst_type)->props, name, &type_val);

        CHECK_TYPE(!same_type(type_val.as.type_type, right_type) && 
                   !struct_or_function_to_nil(type_val.as.type_type, right_type), 
                   prop, "Property and assignment types must match.");

        return RESULT_SUCCESS;
    }
    add_error(compiler, prop, "Property cannot be set.");
    return RESULT_FAILED;
}

static ResultCode compile_node(struct Compiler* compiler, struct Node* node, struct Type** node_type) {
    ResultCode result = RESULT_SUCCESS;
    if (node == NULL) {
        *node_type = make_nil_type();
        return result;
    }
    switch(node->type) {
        //declarations
        case NODE_DECL_VAR: {
            ResultCode result = compile_decl_var_and_get_type(compiler, (DeclVar*)node, node_type);
            //stripping type info away for regular declaration - the type info is need in sequence
            //  assignments/declarations so that's why we need this function
            *node_type = make_nil_type();
            break;
        }
        case NODE_SEQUENCE: {
            struct Sequence* seq = (struct Sequence*)node;

            if (seq->right != NULL) {
                //tracking locals indices to update types if inferred
                int decl_idx[256]; //TODO: this might be bad
                int decl_idx_count = 0;

                //compile all variable declarations
                for (int i = 0; i < seq->left->count; i++) {
                    if (seq->op.type == TOKEN_EQUAL && seq->left->nodes[i]->type == NODE_DECL_VAR) {
                        DeclVar* dv = (DeclVar*)(seq->left->nodes[i]);
                        struct Type* type = NULL;
                        struct Node* decl_var = make_decl_var(dv->name, dv->type, make_nil(make_dummy_token()));
                        COMPILE_NODE(decl_var, &type);
                        int idx = resolve_local(compiler, dv->name);
                        decl_idx[decl_idx_count++] = -1;
                    } else if (seq->op.type == TOKEN_COLON_EQUAL && seq->left->nodes[i]->type == NODE_GET_VAR) {
                        Token name = ((GetVar*)(seq->left->nodes[i]))->name;
                        //Int Type is just a place holder and is replaced later
                        struct Node* decl_var = make_decl_var(name, make_int_type(), make_nil(make_dummy_token()));
                        DeclVar* dv = (DeclVar*)decl_var;
                        struct Type* type = NULL;
                        COMPILE_NODE(decl_var, &type);
                        int idx = resolve_local(compiler, name);
                        decl_idx[decl_idx_count++] = idx;
                    } else {
                        decl_idx[decl_idx_count++] = -1;
                    }
                }

                //otherwise compile right side onto the stack
                //so that left hand side can be assigned to them
                struct Type* right_seq_type = NULL;
                if (seq->right->type == NODE_SEQUENCE) {
                    COMPILE_NODE(seq->right, &right_seq_type);
                } else if (seq->right->type == NODE_LIST) {
                    struct NodeList* nl = (struct NodeList*)(seq->right);
                    struct TypeArray* ta = make_type_array();
                    for (int i = 0; i < nl->count; i++) {
                        struct Type* var_type = NULL;
                        COMPILE_NODE(nl->nodes[i], &var_type);
                        if (var_type != NULL) add_type(ta, var_type);
                    }
                    right_seq_type = (struct Type*)ta;
                }

                if (right_seq_type != NULL) {
                    //update types for variable declarations here using compiled right hand side
                    for (int i = 0; i < decl_idx_count; i++) {
                        if (decl_idx[i] != -1) {
                            compiler->locals[decl_idx[i]].type = ((struct TypeArray*)right_seq_type)->types[i];
                        }
                    }

                    //x, y, z = 1, 2, 3
                    //[1][2][3]
                    //setting variables/props/elements in reverse order to match stack order
                    struct TypeArray* left_seq_type = make_type_array();
                    for (int i = 0; i < seq->left->count; i++) {
                        switch (seq->left->nodes[i]->type) {
                            case NODE_GET_ELEMENT: {
                                GetElement* ge = (GetElement*)(seq->left->nodes[i]);

                                struct Type* left_type = NULL;
                                COMPILE_NODE(ge->left, &left_type);
                                struct Type* idx_type = NULL;
                                COMPILE_NODE(ge->idx, &idx_type);

                                int depth = seq->left->count - 1 - i;
                                struct Type* right_type = ((struct TypeArray*)right_seq_type)->types[i];
                                
                                if (left_type != NULL && idx_type != NULL) {
                                    if (compile_set_element(compiler, ge->name, left_type, idx_type, right_type, depth) == RESULT_FAILED)
                                        result = RESULT_FAILED;
                                }

                                add_type(left_seq_type, right_type);

                                break;
                            }
                            case NODE_GET_PROP: {
                                GetProp* gp = (GetProp*)(seq->left->nodes[i]);
                                struct Type* type_inst = NULL;
                                COMPILE_NODE(gp->inst, &type_inst);

                                struct Type* right_type = ((struct TypeArray*)right_seq_type)->types[i];
                                int depth = seq->left->count - 1 - i;

                                if (type_inst != NULL) {
                                    if (compile_set_prop(compiler, gp->prop, type_inst, right_type, depth) == RESULT_FAILED)
                                        result = RESULT_FAILED;
                                }

                                add_type(left_seq_type, right_type);
                                break;
                            }
                            case NODE_GET_VAR: {
                                Token var = ((GetVar*)(seq->left->nodes[i]))->name;
                                struct Type* var_type = resolve_type(compiler, var);
                                add_type(left_seq_type, var_type);
                                set_var_to_stack_idx(compiler, var, seq->left->count - 1 - i);
                                break;
                            }
                            case NODE_DECL_VAR: {
                                Token var = ((DeclVar*)(seq->left->nodes[i]))->name;
                                struct Type* var_type = resolve_type(compiler, var);
                                add_type(left_seq_type, var_type);
                                set_var_to_stack_idx(compiler, var, seq->left->count - 1 - i);
                                break;
                            }
                            default:
                                break;
                        }
                    }

                    EMIT_ERROR_IF(!same_type((struct Type*)left_seq_type, (struct Type*)right_seq_type),
                                  seq->op, "All variable types and their assignment types must match.");

                    *node_type = (struct Type*)left_seq_type;
                }

            } else {
                struct TypeArray* types = make_type_array();
                for (int i = 0; i < seq->left->count; i++) {
                    struct Type* t = NULL;
                    COMPILE_NODE(seq->left->nodes[i], &t);
                    if (t == NULL) continue;

                    //unwrap type array and insert 
                    if (t->type == TYPE_ARRAY) {
                        for (int i = 0; i < ((struct TypeArray*)t)->count; i++) {
                            add_type(types, ((struct TypeArray*)t)->types[i]);
                        }
                    } else {
                        add_type(types, t);
                    }
                }
                *node_type = (struct Type*)types;
            }

            break;
        }
        case NODE_FUN: {
            DeclFun* df = (DeclFun*)node;

            if (df->anonymous) {
                //assign to variable if assigned
                int set_idx = 0;
                if (df->name.length != 0) {
                    EMIT_ERROR_IF(declared_in_scope(compiler, df->name), df->name, "Identifier already defined.");
                    set_idx = add_local(compiler, df->name, make_infer_type());
                }

                //check for invalid parameter types
                struct TypeFun* fun_type = (struct TypeFun*)(df->type);
                for (int i = 0; i < fun_type->params->count; i++) {
                    EMIT_ERROR_IF(fun_type->params->types[i]->type == TYPE_NIL, df->name, "Parameter identifier type invalid.");
                }
            }

            //compile function using new compiler
            struct Compiler func_comp;
            init_compiler(&func_comp, df->name.start, df->name.length, df->parameters->count, df->name, df->type);
            add_node(df->parameters, df->body);
            struct TypeArray* inner_ret_types = NULL;
            if (compile_function(&func_comp, df->parameters, &inner_ret_types) == RESULT_FAILED)
                result = RESULT_FAILED;

            //check types
            if (inner_ret_types != NULL) {
                struct TypeFun* typefun = (struct TypeFun*)df->type;
                EMIT_ERROR_IF(inner_ret_types->count == 0 &&
                              typefun->returns->count != 1 &&
                              typefun->returns->types[0]->type != TYPE_NIL, 
                              df->name, "A function with no return must declared with no return type.");

                for (int i = 0; i < inner_ret_types->count; i++) {
                    struct Type* inner_type = inner_ret_types->types[i];
                    EMIT_ERROR_IF(!same_type((struct Type*)(typefun->returns), inner_type), 
                                  df->name, "Return type must match type in anonymous function declaration.");
                }
            }

            emit_byte(compiler, OP_FUN);
            emit_short(compiler, add_constant(compiler, to_function(func_comp.function)));
            emit_byte(compiler, func_comp.upvalue_count);

            if (df->anonymous) {
                //only allow upvalues in anonymous functions
                for (int i = 0; i < func_comp.upvalue_count; i++) {
                    emit_byte(compiler, func_comp.upvalues[i].is_local);
                    emit_byte(compiler, func_comp.upvalues[i].index);
                }
            } else {
                //don't allow upvalues in static functions. Add as global.
                EMIT_ERROR_IF(func_comp.upvalue_count > 0, df->name,
                              "Functions cannot capture values.  To create closures, create an anonymous function.");
                emit_byte(compiler, OP_ADD_GLOBAL);
            }

            *node_type = df->type;
            free_compiler(&func_comp);

            break;
        }
        case NODE_STRUCT: {
            struct DeclStruct* dc = (struct DeclStruct*)node;

            //compile super so that it's on the stack
            struct Type* super_type = NULL;
            if (dc->super != NULL) {
                COMPILE_NODE(dc->super, &super_type);
            } else {
                emit_byte(compiler, OP_NIL);
            }

            //set up object
            struct ObjString* struct_string = make_string(dc->name.start, dc->name.length);
            push_root(to_string(struct_string));
            emit_byte(compiler, OP_STRUCT);
            emit_short(compiler, add_constant(compiler, to_string(struct_string)));

            //Get type already completely defined in parser
            Value v;
            get_entry(&compiler->globals, struct_string, &v);
            pop_root(); //struct_string
            struct TypeStruct* klass_type = (struct TypeStruct*)(v.as.type_type);

            //add struct properties
            for (int i = 0; i < dc->decls->count; i++) {
                struct Node* node = dc->decls->nodes[i];
                DeclVar* dv = (DeclVar*)node;
                struct ObjString* prop_name = make_string(dv->name.start, dv->name.length);

                push_root(to_string(prop_name));

                struct TypeArray* class_ret_types = make_type_array();

                start_scope(compiler);
                struct Type* right_type = NULL;
                //need type info, so can't call 'compile_node' (which returns a 'nil' type for variable declarations)
                if (compile_decl_var_and_get_type(compiler, (DeclVar*)node, &right_type) == RESULT_FAILED) {
                    result = RESULT_FAILED;
                }

                if (right_type != NULL) {
                    //update types in TypeStruct if property type is inferred
                    Value v;
                    get_entry(&klass_type->props, prop_name, &v);
                    if (v.as.type_type->type == TYPE_INFER) {
                        set_entry(&klass_type->props, prop_name, to_type(right_type));
                    }

                    Value left_val_type;
                    get_entry(&klass_type->props, prop_name, &left_val_type);
                    EMIT_ERROR_IF(!same_type(left_val_type.as.type_type, right_type) && 
                               !struct_or_function_to_nil(left_val_type.as.type_type, right_type), 
                               dv->name, "Property type must match right hand side.");

                    emit_byte(compiler, OP_ADD_PROP);
                    emit_short(compiler, add_constant(compiler, to_string(prop_name)));
                }

                end_scope(compiler);

                pop_root();
            }

            //set globals in vm
            emit_byte(compiler, OP_ADD_GLOBAL);

            *node_type = (struct Type*)klass_type;

            break;
        }
        case NODE_ENUM: {
            struct DeclEnum* de = (struct DeclEnum*)node;

            //set up objects
            struct ObjEnum* obj_enum = make_enum(de->name);
            push_root(to_enum(obj_enum));

            //fill up &obj_enum->props
            int count = de->decls->count;
            for (int i = 0; i < count; i++) {
                DeclVar* dv = (DeclVar*)(de->decls->nodes[i]);
                struct ObjString* prop_name = make_string(dv->name.start, dv->name.length);
                push_root(to_string(prop_name));
                set_entry(&obj_enum->props, prop_name, to_integer(i));
                pop_root();
            }

            emit_byte(compiler, OP_ENUM);
            emit_short(compiler, add_constant(compiler, to_enum(obj_enum)));

            //set globals in vm
            emit_byte(compiler, OP_ADD_GLOBAL);

            //Get type already completely defined in parser
            Value v;
            get_entry(&compiler->globals, obj_enum->name, &v);
            *node_type = v.as.type_type;

            pop_root(); //struct ObjEnum* 'obj_enum'
            break;
        }
        //statements
        case NODE_EXPR_STMT: {
            ExprStmt* es = (ExprStmt*)node;
            struct Type* type;
            COMPILE_NODE(es->expr, &type);

            //pop multiple times if sequence of Declarations....
            if (type != NULL && type->type == TYPE_ARRAY) {
                for (int i = 0; i < ((struct TypeArray*)type)->count; i++) {
                    emit_byte(compiler, OP_POP);
                }
            //while/if-else etc return VAL_NIL (and leave nothing on stack)
            //should have those also push a nil on the stack so that everything
            //is popped for consistency...
            } else if (type != NULL && type->type != TYPE_NIL) {
                emit_byte(compiler, OP_POP);
            }

            *node_type = make_nil_type();
            break;
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            start_scope(compiler);
            for (int i = 0; i < block->decl_list->count; i++) {
                struct Type* type;
                COMPILE_NODE(block->decl_list->nodes[i], &type);
            }
            end_scope(compiler);
            *node_type = make_nil_type();
            break;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            struct Type* cond = NULL;
            COMPILE_NODE(ie->condition, &cond);

            EMIT_ERROR_IF(cond == NULL || cond->type != TYPE_BOOL, ie->name, 
                       "Condition must evaluate to boolean.");

            int jump_then = emit_jump(compiler, OP_JUMP_IF_FALSE); 

            emit_byte(compiler, OP_POP);
            struct Type* then_type;
            COMPILE_NODE(ie->then_block, &then_type);
            int jump_else = emit_jump(compiler, OP_JUMP);

            patch_jump(compiler, jump_then);
            emit_byte(compiler, OP_POP);

            struct Type* else_type;
            COMPILE_NODE(ie->else_block, &else_type);
            patch_jump(compiler, jump_else);

            *node_type = make_nil_type();
            break;
        }
        case NODE_WHEN: {
            struct When* when = (struct When*)node;
            int* jump_ends = (int*)malloc(MAX_IS_STATEMENTS * sizeof(int));
            int je_count = 0;

            for (int i = 0; i < when->cases->count; i++) {
                IfElse* kase = (IfElse*)(when->cases->nodes[i]);
                struct Type* cond_type = NULL;
                COMPILE_NODE(kase->condition, &cond_type);
                EMIT_ERROR_IF(cond_type == NULL || cond_type->type != TYPE_BOOL, kase->name, 
                        "The expressions after 'when' and 'is' must be comparable using a '==' operator.");

                int jump_then = emit_jump(compiler, OP_JUMP_IF_FALSE);
                emit_byte(compiler, OP_POP);

                struct Type* then_type;
                COMPILE_NODE(kase->then_block, &then_type);
                int jump_end = emit_jump(compiler, OP_JUMP);

                if (je_count < MAX_IS_STATEMENTS) {
                    jump_ends[je_count] = jump_end; 
                    je_count++;
                } else {
                    EMIT_ERROR_IF(true, when->name, "Limit of 256 'is' and 'else' cases in a 'when' statement.");
                }

                patch_jump(compiler, jump_then);
                emit_byte(compiler, OP_POP);
            }

            for (int i = 0; i < je_count; i++) {
                patch_jump(compiler, jump_ends[i]);
            }

            free(jump_ends);
            *node_type = make_nil_type();
            break;
        }
        case NODE_WHILE: {
            While* wh = (While*)node;
            int start = compiler->function->chunk.count;
            struct Type* cond = NULL;
            COMPILE_NODE(wh->condition, &cond);
            EMIT_ERROR_IF(cond == NULL || cond->type != TYPE_BOOL, wh->name,
                       "Condition must evaluate to boolean.");

            int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);

            emit_byte(compiler, OP_POP);
            struct Type* then_block;
            COMPILE_NODE(wh->then_block, &then_block);

            //+3 to include OP_JUMP_BACK and uint16_t for jump distance
            int from = compiler->function->chunk.count + 3;
            emit_jump_by(compiler, OP_JUMP_BACK, from - start);
            patch_jump(compiler, false_jump);   
            emit_byte(compiler, OP_POP);

            *node_type = make_nil_type();
            break;
        }
        case NODE_FOR: {
            For* fo = (For*)node;
            start_scope(compiler);
           
            //initializer
            struct Type* init;
            COMPILE_NODE(fo->initializer, &init);

            //condition
            int condition_start = compiler->function->chunk.count;
            struct Type* cond = NULL;
            COMPILE_NODE(fo->condition, &cond);
            EMIT_ERROR_IF(cond == NULL || cond->type != TYPE_BOOL, fo->name, "Condition must evaluate to boolean.");

            int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
            int body_jump = emit_jump(compiler, OP_JUMP);

            //update
            int update_start = compiler->function->chunk.count;
            if (fo->update) {
                struct Type* up;
                COMPILE_NODE(fo->update, &up);
            }
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->function->chunk.count + 3 - condition_start);

            //body
            patch_jump(compiler, body_jump);
            emit_byte(compiler, OP_POP); //pop condition if true

            struct Type* then_type;
            COMPILE_NODE(fo->then_block, &then_type);
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->function->chunk.count + 3 - update_start);

            patch_jump(compiler, exit_jump);
            emit_byte(compiler, OP_POP); //pop condition if false

            end_scope(compiler);
            *node_type = make_nil_type();
            break;
        }
        case NODE_RETURN: {
            Return* ret = (Return*)node;
            struct Type* type = NULL;
            COMPILE_NODE(ret->right, &type);
            int return_count = 0;
            if (type != NULL && type->type == TYPE_ARRAY) {
                return_count = ((struct TypeArray*)type)->count; 
            }
            emit_byte(compiler, OP_RETURN);
            emit_byte(compiler, return_count);

            add_type(compiler->return_types, type);
            *node_type = type;
            break;
        }
        //expressions
        case NODE_LITERAL:
            if (compile_literal(compiler, node, node_type) == RESULT_FAILED) result = RESULT_FAILED;
            break;
        case NODE_BINARY:       
            if (compile_binary(compiler, node, node_type) == RESULT_FAILED) result = RESULT_FAILED;
            break;
        case NODE_LOGICAL:
            if (compile_logical(compiler, node, node_type) == RESULT_FAILED) result = RESULT_FAILED;
            break;
        case NODE_UNARY:
            if (compile_unary(compiler, node, node_type) == RESULT_FAILED) result = RESULT_FAILED;
            break;
        case NODE_GET_PROP: {
            GetProp* gp = (GetProp*)node;
            struct Type* type_inst = NULL;
            COMPILE_NODE(gp->inst, &type_inst);

            if (type_inst == NULL) {
                EMIT_ERROR_IF(type_inst == NULL, gp->prop, "Trying to access property from invalid object.");
            } else if (type_inst->type == TYPE_LIST) {
                EMIT_ERROR_IF(!same_token_literal(gp->prop, make_token(TOKEN_DUMMY, 0, "size", 4)), gp->prop, "Property doesn't exist on Lists.");
                emit_byte(compiler, OP_GET_SIZE);
                *node_type = make_int_type();
            } else if (type_inst->type == TYPE_DECL) {
                struct TypeDecl* td = (struct TypeDecl*)type_inst;
                //should be ObjEnum on the stack at this point
                //Enums are a special case of properties
                if (td->custom_type->type == TYPE_ENUM) {
                    struct TypeEnum* te = (struct TypeEnum*)(td->custom_type);
                    emit_byte(compiler, OP_GET_PROP);
                    struct ObjString* name = make_string(gp->prop.start, gp->prop.length);
                    push_root(to_string(name));
                    emit_short(compiler, add_constant(compiler, to_string(name))); 
                    pop_root();

                    Value type_val;
                    EMIT_ERROR_IF(!get_entry(&te->props, name, &type_val), gp->prop, "Constant doesn't exist in enum.");

                    *node_type = (struct Type*)te;
                } else {
                    EMIT_ERROR_IF(true, gp->prop, "Can only use dot notation to select elements in enumerations.");
                }
            } else if (type_inst->type == TYPE_STRING) {
                EMIT_ERROR_IF(!same_token_literal(gp->prop, make_token(TOKEN_DUMMY, 0, "size", 4)), gp->prop, "Property doesn't exist on strings.");
                emit_byte(compiler, OP_GET_SIZE);
                *node_type = make_int_type();
            } else if (type_inst->type == TYPE_MAP) {
                if (same_token_literal(gp->prop, make_token(TOKEN_DUMMY, 0, "keys", 4))) {
                    emit_byte(compiler, OP_GET_KEYS);
                    *node_type = make_list_type(make_string_type());
                } else if (same_token_literal(gp->prop, make_token(TOKEN_DUMMY, 0, "values", 6))) {
                    emit_byte(compiler, OP_GET_VALUES);
                    *node_type = make_list_type(((struct TypeMap*)type_inst)->type);
                } else {
                    EMIT_ERROR_IF(true, gp->prop, "Property doesn't exist on Map.");
                }
            } else if (type_inst->type == TYPE_STRUCT) {
                emit_byte(compiler, OP_GET_PROP);
                struct ObjString* name = make_string(gp->prop.start, gp->prop.length);
                push_root(to_string(name));
                emit_short(compiler, add_constant(compiler, to_string(name))); 
                pop_root();

                Value type_val = to_nil();
                struct Type* current = type_inst;
                while (current != NULL) {
                    struct TypeStruct* tc = (struct TypeStruct*)current;
                    if (get_entry(&tc->props, name, &type_val)) break;
                    current = tc->super;
                }

                EMIT_ERROR_IF(current == NULL, gp->prop, "Property not found on object.");
                *node_type = type_val.as.type_type;
            } else {
                EMIT_ERROR_IF(true, gp->prop, "Object does not have properties that can be accessed.");
            }

            break;
        }
        case NODE_SET_PROP: {
            SetProp* sp = (SetProp*)node;
            struct Type* right_type = NULL;
            COMPILE_NODE(sp->right, &right_type);
            struct Type* type_inst = NULL;
            COMPILE_NODE(((GetProp*)(sp->inst))->inst, &type_inst);
            Token prop = ((GetProp*)sp->inst)->prop;

            int depth = 0;
            if (right_type != NULL && type_inst != NULL) {
                if (compile_set_prop(compiler, prop, type_inst, right_type, depth) == RESULT_FAILED) result = RESULT_FAILED;
            }

            *node_type = right_type;
            break;
        }
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;

            int idx = resolve_local(compiler, gv->name);
            OpCode op = OP_GET_LOCAL;
            if (idx == -1) {
                idx = resolve_upvalue(compiler, gv->name);
                op = OP_GET_UPVALUE;
            }

            //local or upvalue
            if (idx != -1) {
                emit_byte(compiler, op);
                emit_byte(compiler, idx);
                *node_type = resolve_type(compiler, gv->name);
            } else {
                struct ObjString* name = make_string(gv->name.start, gv->name.length);
                push_root(to_string(name));
                Value v;
                //global
                if (get_entry(&script_compiler->globals, name, &v)) {
                    emit_byte(compiler, OP_GET_GLOBAL);
                    emit_short(compiler, add_constant(compiler, to_string(name)));
                    if (v.as.type_type->type == TYPE_STRUCT || v.as.type_type->type == TYPE_ENUM) {
                        *node_type = make_decl_type(v.as.type_type);
                    } else {
                        *node_type = v.as.type_type;
                    }
                } else {
                    EMIT_ERROR_IF(true, gv->name, "Attempting to access undeclared variable.");
                }
                pop_root();
            }

            break;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            struct Type* right_type = NULL;
            COMPILE_NODE(sv->right, &right_type);

            Token var = ((GetVar*)(sv->left))->name;
            struct Type* var_type = resolve_type(compiler, var);
            *node_type = var_type;

            EMIT_ERROR_IF(right_type == NULL || (!same_type(var_type, right_type) && !struct_or_function_to_nil(var_type, right_type)), 
                       var, "Right side type must match variable type.");

            int depth = 0;
            if (set_var_to_stack_idx(compiler, var, depth) == RESULT_FAILED) {
                result = RESULT_FAILED;
                EMIT_ERROR_IF(true, var, "Local variable not declared.");
            }

            break;
        }
        case NODE_SLICE_STRING: {
            SliceString* ss = (SliceString*)node;
            struct Type* left_type = NULL;
            COMPILE_NODE(ss->left, &left_type);
            struct Type* start_idx_type = NULL;
            COMPILE_NODE(ss->start_idx, &start_idx_type);
            struct Type* end_idx_type = NULL;
            COMPILE_NODE(ss->end_idx, &end_idx_type);

            EMIT_ERROR_IF(left_type == NULL || left_type->type != TYPE_STRING, ss->name, "Slicing can only be used on strings.");
            EMIT_ERROR_IF(start_idx_type == NULL || start_idx_type->type != TYPE_INT, ss->name, "Start index when slicing must be an integer.");
            EMIT_ERROR_IF(end_idx_type == NULL || end_idx_type->type != TYPE_INT, ss->name, "End index when slicing must be an integer.");

            emit_byte(compiler, OP_SLICE);
            *node_type = left_type;
            break;
        }
        case NODE_GET_ELEMENT: {
            GetElement* get_idx = (GetElement*)node;
            struct Type* left_type = NULL;
            COMPILE_NODE(get_idx->left, &left_type);
            struct Type* idx_type = NULL;
            COMPILE_NODE(get_idx->idx, &idx_type);

            if (left_type == NULL) {
                EMIT_ERROR_IF(true, get_idx->name, "[] access must be used on a list or map type.");
            } else if (left_type->type == TYPE_STRING) {
                EMIT_ERROR_IF(idx_type->type != TYPE_INT, get_idx->name, "Index must be integer type.");

                emit_byte(compiler, OP_GET_ELEMENT);
                *node_type = left_type;
            } else if (left_type->type == TYPE_LIST) {
                EMIT_ERROR_IF(idx_type->type != TYPE_INT, get_idx->name, "Index must be integer type.");

                emit_byte(compiler, OP_GET_ELEMENT);
                *node_type = ((struct TypeList*)left_type)->type;
            } else if (left_type->type == TYPE_MAP) {
                EMIT_ERROR_IF(idx_type->type != TYPE_STRING, get_idx->name, "Key must be string type.");

                emit_byte(compiler, OP_GET_ELEMENT);
                *node_type = ((struct TypeList*)left_type)->type;
            } else {
                EMIT_ERROR_IF(true, get_idx->name, "[] access must be used on a list or map type.");
            }

            break;
        }
        case NODE_SET_ELEMENT: {
            SetElement* set_elem = (SetElement*)node;
            GetElement* get_elem = (GetElement*)(set_elem->left);

            struct Type* right_type = NULL;
            COMPILE_NODE(set_elem->right, &right_type);
            struct Type* left_type = NULL;
            COMPILE_NODE(get_elem->left, &left_type);
            struct Type* idx_type = NULL;
            COMPILE_NODE(get_elem->idx, &idx_type);

            int depth = 0;
            if (right_type != NULL && left_type != NULL && idx_type != NULL) {
                if (compile_set_element(compiler, get_elem->name, left_type, idx_type, right_type, depth) == RESULT_FAILED)
                    result = RESULT_FAILED;
            }

            *node_type = right_type;
            break;
        }
        case NODE_CONTAINER: {
            struct DeclContainer* dc = (struct DeclContainer*)node;
            if (dc->type->type == TYPE_LIST) {
                emit_byte(compiler, OP_LIST);
                *node_type = dc->type;
            } else if (dc->type->type == TYPE_MAP) {
                emit_byte(compiler, OP_MAP);
                *node_type = dc->type;
            } else {
                EMIT_ERROR_IF(true, dc->name, "Invalid identifier for container.");
            }

            break;
        }
        case NODE_CALL: {
            Call* call = (Call*)node;

            struct Type* type = NULL;
            COMPILE_NODE(call->left, &type);

            if (type == NULL) {
                EMIT_ERROR_IF(true, call->name, "Calls can only be made on functions, structs, List and Map.");
            } else if (type->type == TYPE_DECL) {
                struct TypeDecl* td = (struct TypeDecl*)type;
                if (td->custom_type->type == TYPE_STRUCT) {
                    struct TypeStruct* type_struct = (struct TypeStruct*)(td->custom_type);
                    emit_byte(compiler, OP_INSTANCE);
                    *node_type = (struct Type*)type_struct;
                } else {
                    EMIT_ERROR_IF(true, call->name, "Calls can only be used on functions or to instantiate structs.");
                }
            } else if (type->type == TYPE_FUN) {
                struct TypeFun* type_fun = (struct TypeFun*)type;
                struct TypeArray* params = (type_fun->params);

                if (call->arguments->count == params->count) {
                    for (int i = 0; i < params->count; i++) {
                        struct Type* arg_type = NULL;
                        COMPILE_NODE(call->arguments->nodes[i], &arg_type);

                        struct Type* param_type = params->types[i];

                        EMIT_ERROR_IF(arg_type != NULL && !same_type(param_type, arg_type), call->name, "Argument type must match parameter type.");
                    }

                    emit_byte(compiler, OP_CALL);
                    emit_byte(compiler, (uint8_t)(call->arguments->count));

                    if (type_fun->returns->count == 1) *node_type = type_fun->returns->types[0];
                    else *node_type = (struct Type*)(type_fun->returns);

                } else {
                    EMIT_ERROR_IF(true, call->name, "Argument count must match function parameter count.");
                }

            } else {
                EMIT_ERROR_IF(true, call->name, "Calls can only be made on functions, structs, List and Map.");
            }

            break;
        }
        case NODE_NIL: {
            Nil* nil = (Nil*)node;
            emit_byte(compiler, OP_NIL);
            *node_type =  make_nil_type();
            break;
        }
        case NODE_CAST: {
            Cast* cast = (Cast*)node;
            struct Type* left = NULL;
            COMPILE_NODE_OLD(cast->left, &left);

            if (left != NULL && is_primitive(left) && is_primitive(cast->type)) {
                EMIT_ERROR_IF(left->type == TYPE_NIL && cast->type->type != TYPE_STRING, cast->name, "'nil' types can only be cast to string.");
                EMIT_ERROR_IF(cast->type->type == TYPE_NIL, cast->name, "Cannot cast to 'nil' type.");
                EMIT_ERROR_IF(left->type == cast->type->type, cast->name, "Cannot cast to same type.");
                emit_byte(compiler, OP_CAST);
                emit_short(compiler, cast->type->type);
                *node_type = cast->type;
            } else if (left != NULL && left->type == TYPE_STRUCT && cast->type->type == TYPE_STRUCT) {
                struct TypeStruct* from = (struct TypeStruct*)(left);
                struct TypeStruct* to = (struct TypeStruct*)(cast->type);

                EMIT_ERROR_IF(same_token_literal(from->name, to->name), cast->name, "Attempting to cast to own type.");

                bool cast_up = is_substruct(from, to);
                bool cast_down = is_substruct(to, from);
                bool valid_cast = cast_up || cast_down;

                EMIT_ERROR_IF(!(cast_up || cast_down), cast->name, "Struct instances must be cast only to superstructs or substructs.");

                emit_byte(compiler, OP_CAST);
                struct ObjString* to_struct = make_string(to->name.start, to->name.length);
                push_root(to_string(to_struct));
                emit_short(compiler, add_constant(compiler, to_string(to_struct)));
                pop_root();

                *node_type =  cast->type;
            } else {
                EMIT_ERROR_IF(true, cast->name, "Type casts with 'as' must be applied to structure types or primitive types.");
            }

            break;
        }
    } 

    return result;
}


void init_compiler(struct Compiler* compiler, const char* start, int length, int parameter_count, Token name, struct Type* type) {
    struct ObjString* fun_name = make_string(start, length);
    push_root(to_string(fun_name));
    compiler->scope_depth = 0;
    compiler->locals_count = 0;
    add_local(compiler, name, type); //first slot is for function
    compiler->errors = (struct Error*)malloc(MAX_ERRORS * sizeof(struct Error));
    compiler->error_count = 0;
    compiler->function = make_function(fun_name, parameter_count);
    push_root(to_function(compiler->function));
    compiler->enclosing = NULL;
    compiler->upvalue_count = 0;
    compiler->types = NULL;
    compiler->nodes = NULL;
    init_table(&compiler->globals);

    compiler->enclosing = current_compiler;
    compiler->return_types = NULL;
    current_compiler = compiler;

    pop_root();
    pop_root();
}

void free_compiler(struct Compiler* compiler) {
    if (compiler->enclosing != NULL) {
        copy_errors(compiler->enclosing, compiler);
    }
    free(compiler->errors);
    while (compiler->types != NULL) {
        struct Type* previous = compiler->types;
        compiler->types = compiler->types->next;
        free_type(previous);
    }
    while (compiler->nodes != NULL) {
        struct Node* previous = compiler->nodes;
        compiler->nodes = compiler->nodes->next;
        free_node(previous);
    }
    free_table(&compiler->globals);
    current_compiler = compiler->enclosing;
}

static ResultCode compile_function(struct Compiler* compiler, struct NodeList* nl, struct TypeArray** type_array) {
    compiler->return_types = make_type_array();
    for (int i = 0; i < nl->count; i++) {
        struct Type* type;
        ResultCode result = compile_node(compiler, nl->nodes[i], &type);
        if (result == RESULT_FAILED) {
            add_error(compiler, make_dummy_token(), "Failed to compile function.");
            return RESULT_FAILED;
        }
    }

    if (compiler->return_types->count == 0) {
        emit_byte(compiler, OP_NIL);
        emit_byte(compiler, OP_RETURN);
        emit_byte(compiler, 1);
    }

    *type_array = compiler->return_types;

    return RESULT_SUCCESS;
}


ResultCode define_native(struct Compiler* compiler, const char* name, ResultCode (*function)(int, Value*, struct ValueArray*), struct Type* type) {
    //set globals in compiler for checks
    struct ObjString* native_string = make_string(name, strlen(name));
    push_root(to_string(native_string));
    Value v;
    if (get_entry(&compiler->globals, native_string, &v)) {
        pop_root();
        add_error(compiler, make_dummy_token(), "The identifier for this function is already used.");
        return RESULT_FAILED;
    }
    set_entry(&compiler->globals, native_string, to_type(type));
    pop_root();

    emit_byte(compiler, OP_NATIVE);
    Value native = to_native(make_native(native_string, function));
    push_root(native);
    emit_short(compiler, add_constant(compiler, native));
    pop_root();
    emit_byte(compiler, OP_ADD_GLOBAL);

    return RESULT_SUCCESS;
}

ResultCode compile_script(struct Compiler* compiler, struct NodeList* nl) {
    //TODO:'script_compiler' is a global currently used to get access to top compiler globals table
    //  this is kind of messy
    script_compiler = compiler;
   
    //this part is mostly the same as compile_function, except
    //that return opcodes for functions is emitted 
    ResultCode result;
    compiler->return_types = make_type_array();

    int start_locals_count = compiler->locals_count;

    for (int i = 0; i < nl->count; i++) {
        struct Type* type;
        result = compile_node(compiler, nl->nodes[i], &type);
        if (result == RESULT_FAILED) {
            add_error(compiler, make_dummy_token(), "Failed to compile script.");
            break;
        }
    }


    //halt programming rather than OP_RETURN byte sequence so that REPL works
    //vm stack and open_upvalues is cleared in repl/run_script functions
    emit_byte(compiler, OP_HALT);

    if (result == RESULT_FAILED || compiler->error_count > 0) {
        for (int i = 0; i < compiler->error_count; i++) {
            printf("[line %d] %s\n", compiler->errors[i].token.line, compiler->errors[i].message);
        }

        //reset errors/locals count
        compiler->locals_count = start_locals_count;
        compiler->error_count = 0;

        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}


void print_locals(struct Compiler* compiler) {
    printf("total [%d] ", compiler->locals_count);
    for (int i = 0; i < compiler->locals_count; i ++) {
        Local* local = &compiler->locals[i];
        printf("%.*s, ", local->name.length, local->name.start);
    }
}

