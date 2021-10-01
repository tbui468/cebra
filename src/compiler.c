#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "compiler.h"
#include "memory.h"
#include "obj.h"

struct Compiler* current_compiler = NULL;
struct Compiler* class_compiler = NULL;
static ResultCode compile_node(struct Compiler* compiler, struct Node* node, struct TypeArray* ret_types, struct Type** node_type);
static ResultCode compile_function(struct Compiler* compiler, struct NodeList* nl, struct TypeArray** type_array);

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
    return RESULT_FAILED;
}

static ResultCode compile_unary(struct Compiler* compiler, struct Node* node, struct Type** node_type) {
    Unary* unary = (Unary*)node;
    struct Type* type;
    if (compile_node(compiler, unary->right, NULL, &type) == RESULT_FAILED) return RESULT_FAILED;
    emit_byte(compiler, OP_NEGATE);
    *node_type = type;
    return RESULT_SUCCESS;
}

static ResultCode compile_logical(struct Compiler* compiler, struct Node* node, struct Type** node_type) {
    Logical* logical = (Logical*)node;

    if (logical->name.type == TOKEN_AND) {
        struct Type* left_type;
        if (compile_node(compiler, logical->left, NULL, &left_type) == RESULT_FAILED) return RESULT_FAILED;
        int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
        struct Type* right_type;
        if (compile_node(compiler, logical->right, NULL, &right_type) == RESULT_FAILED) return RESULT_FAILED;
        patch_jump(compiler, false_jump);
        if (!same_type(left_type, right_type)) {
            add_error(compiler, logical->name, "Left and right types must match.");
            return RESULT_FAILED;
        }
        *node_type = make_bool_type();
        return RESULT_SUCCESS;
    }

    if (logical->name.type == TOKEN_OR) {
        struct Type* left_type;
        if (compile_node(compiler, logical->left, NULL, &left_type) == RESULT_FAILED) return RESULT_FAILED;
        int true_jump = emit_jump(compiler, OP_JUMP_IF_TRUE);
        emit_byte(compiler, OP_POP);
        struct Type* right_type;
        if (compile_node(compiler, logical->right, NULL, &right_type) == RESULT_FAILED) return RESULT_FAILED;
        patch_jump(compiler, true_jump);
        if (!same_type(left_type, right_type)) {
            add_error(compiler, logical->name, "Left and right types must match.");
            return RESULT_FAILED;
        }
        *node_type = make_bool_type();
        return RESULT_SUCCESS;
    }

    if (logical->name.type == TOKEN_IN) {
        struct Type* element_type;
        if (compile_node(compiler, logical->left, NULL, &element_type) == RESULT_FAILED) return RESULT_FAILED;
        struct Type* list_type;
        if (compile_node(compiler, logical->right, NULL, &list_type) == RESULT_FAILED) return RESULT_FAILED;

        if (list_type->type != TYPE_LIST) {
            add_error(compiler, logical->name, "Identifier after 'in' must reference a List.");
            return RESULT_FAILED;
        }

        if (!same_type(element_type, ((struct TypeList*)list_type)->type)) {
            add_error(compiler, logical->name, "Type left of 'in' must match List element type.");
            return RESULT_FAILED;
        }

        emit_byte(compiler, OP_IN_LIST);
        *node_type = make_bool_type();
        return RESULT_SUCCESS;
    }

    struct Type* left_type;
    if (compile_node(compiler, logical->left, NULL, &left_type)) return RESULT_FAILED;
    struct Type* right_type;
    if (compile_node(compiler, logical->right, NULL, &right_type)) return RESULT_FAILED;

    if (!same_type(left_type, right_type)) {
        add_error(compiler, logical->name, "Left and right types must match.");
        return RESULT_FAILED;
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
    *node_type = make_bool_type();
    return RESULT_SUCCESS;
}

static ResultCode compile_binary(struct Compiler* compiler, struct Node* node, struct Type** node_type) {
    Binary* binary = (Binary*)node;

    struct Type* type1;
    if (compile_node(compiler, binary->left, NULL, &type1) == RESULT_FAILED) return RESULT_FAILED;
    struct Type* type2;
    if (compile_node(compiler, binary->right, NULL, &type2) == RESULT_FAILED) return RESULT_FAILED;
    if (!same_type(type1, type2)) {
        add_error(compiler, binary->name, "Left and right types must match.");
        return RESULT_FAILED;
    }
    switch(binary->name.type) {
        case TOKEN_PLUS: emit_byte(compiler, OP_ADD); break;
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
        for (int i = current->locals_count - 1; i >= 0; i--) {
            Local* local = &current->locals[i];
            if (local->name.length == name.length && memcmp(local->name.start, name.start, name.length) == 0) {
                return current->locals[i].type;
            }
        }
        current = current->enclosing;
    } while(current != NULL);

    add_error(compiler, name, "Local variable not declared.");

    return NULL;
}

static bool declared_in_scope(struct Compiler* compiler, Token name) {
    for (int i = compiler->locals_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->name.length == name.length && 
            memcmp(local->name.start, name.start, name.length) == 0) {
            if (compiler->scope_depth == local->depth) return true; 
            else return false;
        }
    }

    return false;
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

static void resolve_self_ref_type(struct TypeClass* sc) {
    for (int i = 0; i < sc->props.capacity; i++) {
        struct Pair* pair = &sc->props.pairs[i];
        if (pair->key != NULL && pair->value.as.type_type->type == TYPE_CLASS) {
            struct TypeClass* sc2 = (struct TypeClass*)(pair->value.as.type_type);
            if (sc->klass.length == sc2->klass.length && 
                memcmp(sc->klass.start, sc2->klass.start, sc->klass.length) == 0) {
                set_table(&sc->props, pair->key, to_type((struct Type*)sc)); 
            }
        } else if(pair->key != NULL && pair->value.as.type_type->type == TYPE_DECL) {
            set_table(&sc->props, pair->key, to_type((struct Type*)sc));
        }
    }
}


static ResultCode compile_node(struct Compiler* compiler, struct Node* node, struct TypeArray* ret_types, struct Type** node_type) {
    if (node == NULL) {
        *node_type = make_nil_type();
        return RESULT_SUCCESS;
    }
    switch(node->type) {
        //declarations
        case NODE_DECL_VAR: {
            DeclVar* dv = (DeclVar*)node;

            struct Type* type = NULL;

            if (declared_in_scope(compiler, dv->name)) {
                add_error(compiler, dv->name, "Identifier already defined.");
                return RESULT_FAILED;
            }

            //inferred type, defined
            if (dv->type->type == TYPE_DECL) {
                int idx = add_local(compiler, dv->name, dv->type);
                if (compile_node(compiler, dv->right, NULL, &type) == RESULT_FAILED) return RESULT_FAILED;
                if (type->type == TYPE_NIL) {
                    add_error(compiler, dv->name, "Inferred type cannot be assigned to 'nil'.");
                    return RESULT_FAILED;
                }
                set_local(compiler, dv->name, type, idx);

                if (type->type == TYPE_CLASS) {
                    resolve_self_ref_type((struct TypeClass*)type);
                }
            }
            
            //explicit type, defined and parameters
            if (dv->type->type != TYPE_DECL) {
                int idx = add_local(compiler, dv->name, dv->type);
                if (compile_node(compiler, dv->right, NULL, &type) == RESULT_FAILED) return RESULT_FAILED;

                if (type->type != TYPE_NIL) {
                    set_local(compiler, dv->name, type, idx);

                    if (dv->type->type == TYPE_CLASS) {
                        resolve_self_ref_type((struct TypeClass*)type);
                        dv->type = type;
                    }

                    if (dv->type->type == TYPE_IDENTIFIER) {
                        dv->type = resolve_type(compiler, dv->name);
                    }

                    if (!same_type(dv->type, type)) {
                        add_error(compiler, dv->name, "Declaration type and right hand side type must match.");
                        return RESULT_FAILED;
                    }
                }

                if (dv->type->type == TYPE_IDENTIFIER) {
                    dv->type = resolve_type(compiler, dv->name);
                }
            }

            *node_type = type;
            return RESULT_SUCCESS;
        }
        case NODE_FUN: {
            DeclFun* df = (DeclFun*)node;

            //TODO: resolve any identifiers in function type 
            //doing the same thing in SET_PROP - need to find a systematic way of
            //resolving identifiers instead of doing in randomly throughout code
            struct TypeFun* fun_type = (struct TypeFun*)(df->type);
            if (fun_type->ret->type == TYPE_IDENTIFIER) {
                fun_type->ret = resolve_type(compiler, ((struct TypeIdentifier*)(fun_type->ret))->identifier);
            }
            struct TypeArray* params = (struct TypeArray*)(fun_type->params);
            for (int i = 0; i < params->count; i++) {
                if (params->types[i]->type == TYPE_IDENTIFIER) {
                    params->types[i] = resolve_type(compiler, ((struct TypeIdentifier*)(params->types[i]))->identifier);
                }
            }

            struct Compiler func_comp;
            init_compiler(&func_comp, df->name.start, df->name.length, df->parameters->count);
            set_local(&func_comp, df->name, df->type, 0);
            add_node(df->parameters, df->body);
            struct TypeArray* inner_ret_types;
            ResultCode result = compile_function(&func_comp, df->parameters, &inner_ret_types); 

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(func_comp.function);
    int i = 0;
    printf("-------------------\n");
    while (i < func_comp.function->chunk.count) {
       OpCode op = func_comp.function->chunk.codes[i++];
       printf("[ %s ]\n", op_to_string(op));
    }
#endif

            copy_errors(compiler, &func_comp);

            struct TypeFun* typefun = (struct TypeFun*)df->type;
            if (inner_ret_types->count == 0 && typefun->ret->type != TYPE_NIL) {
                add_error(compiler, df->name, "Return type must match typenature in function declaration.");
                return RESULT_FAILED;
            }

            struct Type* ret_type = typefun->ret;
            if (ret_type->type == TYPE_IDENTIFIER) {
                ret_type = resolve_type(compiler, ((struct TypeIdentifier*)ret_type)->identifier);
            }
            
            for (int i = 0; i < inner_ret_types->count; i++) {
                struct Type* inner_type = inner_ret_types->types[i];
                if (inner_type->type == TYPE_IDENTIFIER) {
                    inner_type = resolve_type(compiler, ((struct TypeIdentifier*)inner_type)->identifier);
                }
                if (!same_type(ret_type, inner_type)) {
                    add_error(compiler, df->name, "Return type must match typenature in function declaration.");
                    return RESULT_FAILED;
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

            *node_type = df->type;
            return RESULT_SUCCESS;
        }
        case NODE_CLASS: {
            DeclClass* dc = (DeclClass*)node;

            struct ObjString* name = make_string(dc->name.start, dc->name.length);
            push_root(to_string(name));
            struct ObjClass* klass = make_class(name);
            push_root(to_class(klass));

            struct TypeClass* super_type;
            if (dc->super != NULL) {
                struct Type* s;
                if (compile_node(compiler, dc->super, ret_types, &s) == RESULT_FAILED) return RESULT_FAILED;
                super_type = (struct TypeClass*)s;
            } else {
                emit_byte(compiler, OP_NIL);
            }

            emit_byte(compiler, OP_CLASS);
            emit_short(compiler, add_constant(compiler, to_class(klass))); //should be created in vm

            GetVar* super_gv = (GetVar*)(dc->super);
            struct TypeClass* sc = (struct TypeClass*)make_class_type(dc->name);

            //add struct properties
            for (int i = 0; i < dc->decls->count; i++) {
                struct Node* node = dc->decls->nodes[i];
                DeclVar* dv = (DeclVar*)node;
                struct ObjString* prop_name = make_string(dv->name.start, dv->name.length);

                push_root(to_string(prop_name));

                struct Type* class_ret_types = make_array_type();

                start_scope(compiler);
                struct Type* type;
                if (compile_node(compiler, node, (struct TypeArray*)class_ret_types, &type) == RESULT_FAILED) return RESULT_FAILED;

                if (type->type == TYPE_NIL) {
                    struct Type* dv_type = dv->type;
                    if (dv_type->type == TYPE_IDENTIFIER) {
                        dv_type = resolve_type(compiler, ((struct TypeIdentifier*)dv_type)->identifier);
                    }
                    set_table(&sc->props, prop_name, to_type(dv_type));
                } else {
                    set_table(&sc->props, prop_name, to_type(type));
                }


                emit_byte(compiler, OP_ADD_PROP);
                emit_short(compiler, add_constant(compiler, to_string(prop_name)));
                end_scope(compiler);

                pop_root();
            }


            if (dc->super != NULL) {
                //type checking overwritten properties
                for (int i = 0; i < sc->props.capacity; i++) {
                    struct Pair* pair = &sc->props.pairs[i];
                    if (pair->key != NULL) {
                        Value value;
                        bool overwritting = get_from_table(&super_type->props, pair->key, &value);
                        if (overwritting && !same_type(value.as.type_type, pair->value.as.type_type)) {
                            add_error(compiler, dc->name, "Overwritten property must of same type.");
                            return RESULT_FAILED;
                        }
                    }
                }
                //inheriting properties in typenature
                for (int i = 0; i < super_type->props.capacity; i++) {
                    struct Pair* pair = &super_type->props.pairs[i];
                    if (pair->key != NULL) {
                        set_table(&sc->props, pair->key, pair->value);
                    }
                }
            }

            pop_root();
            pop_root();

            *node_type = (struct Type*)sc;
            return RESULT_SUCCESS;
        }
        case NODE_ENUM: {
            struct DeclEnum* de = (struct DeclEnum*)node;

            struct ObjString* name = make_string(de->name.start, de->name.length);
            push_root(to_string(name));
            struct ObjEnum* obj_enum = make_enum(name);
            push_root(to_enum(obj_enum));
            
            emit_byte(compiler, OP_ENUM);
            emit_short(compiler, add_constant(compiler, to_enum(obj_enum)));

            pop_root();
            pop_root();

            int count = de->decls->count;
            emit_byte(compiler, count);

            struct TypeEnum* type = (struct TypeEnum*)make_enum_type(de->name);

            for (int i = 0; i < count; i++) {
                DeclVar* dv = (DeclVar*)(de->decls->nodes[i]);
                struct ObjString* prop_name = make_string(dv->name.start, dv->name.length);
                push_root(to_string(prop_name));
                emit_short(compiler, add_constant(compiler, to_string(prop_name)));
                set_table(&type->props, prop_name, to_type(make_int_type()));
                pop_root();
            }
           
            *node_type = (struct Type*)type;
            return RESULT_SUCCESS;
        }
        //statements
        case NODE_EXPR_STMT: {
            ExprStmt* es = (ExprStmt*)node;
            struct Type* type;
            if (compile_node(compiler, es->expr, ret_types, &type) == RESULT_FAILED) return RESULT_FAILED;

            //while/if-else etc return VAL_NIL (and leave nothing on stack)
            if (type->type != TYPE_NIL) {
                emit_byte(compiler, OP_POP);
            }
            *node_type = make_nil_type();
            return RESULT_SUCCESS;
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            start_scope(compiler);
            for (int i = 0; i < block->decl_list->count; i++) {
                struct Type* type;
                if (compile_node(compiler, block->decl_list->nodes[i], ret_types, &type) == RESULT_FAILED) return RESULT_FAILED;
            }
            end_scope(compiler);
            *node_type = make_nil_type();
            return RESULT_SUCCESS;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            struct Type* cond;
            if (compile_node(compiler, ie->condition, ret_types, &cond) == RESULT_FAILED) return RESULT_FAILED;

            if (cond->type != TYPE_BOOL) {
                add_error(compiler, ie->name, "Condition must evaluate to boolean.");
                return RESULT_FAILED;
            }

            int jump_then = emit_jump(compiler, OP_JUMP_IF_FALSE); 

            emit_byte(compiler, OP_POP);
            struct Type* then_type;
            if (compile_node(compiler, ie->then_block, ret_types, &then_type) == RESULT_FAILED) return RESULT_FAILED;
            int jump_else = emit_jump(compiler, OP_JUMP);

            patch_jump(compiler, jump_then);
            emit_byte(compiler, OP_POP);
            struct Type* else_type;
            if (compile_node(compiler, ie->else_block, ret_types, &else_type) == RESULT_FAILED) return RESULT_FAILED;
            patch_jump(compiler, jump_else);
            *node_type = make_nil_type();
            return RESULT_SUCCESS;
        }
        case NODE_WHILE: {
            While* wh = (While*)node;
            int start = compiler->function->chunk.count;
            struct Type* cond;
            if (compile_node(compiler, wh->condition, ret_types, &cond) == RESULT_FAILED) return RESULT_FAILED;
            if (cond->type != TYPE_BOOL) {
                add_error(compiler, wh->name, "Condition must evaluate to boolean.");
                return RESULT_FAILED;
            }

            int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);

            emit_byte(compiler, OP_POP);
            struct Type* then_block;
            if (compile_node(compiler, wh->then_block, ret_types, &then_block) == RESULT_FAILED) return RESULT_FAILED;

            //+3 to include OP_JUMP_BACK and uint16_t for jump distance
            int from = compiler->function->chunk.count + 3;
            emit_jump_by(compiler, OP_JUMP_BACK, from - start);
            patch_jump(compiler, false_jump);   
            emit_byte(compiler, OP_POP);
            *node_type = make_nil_type();
            return RESULT_SUCCESS;
        }
        case NODE_FOR: {
            For* fo = (For*)node;
            start_scope(compiler);
           
            //initializer
            struct Type* init;
            if (compile_node(compiler, fo->initializer, ret_types, &init) == RESULT_FAILED) return RESULT_FAILED;

            //condition
            int condition_start = compiler->function->chunk.count;
            struct Type* cond;
            if (compile_node(compiler, fo->condition, ret_types, &cond) == RESULT_FAILED) return RESULT_FAILED;
            if (cond->type != TYPE_BOOL) {
                add_error(compiler, fo->name, "Condition must evaluate to boolean.");
                return RESULT_FAILED;
            }

            int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
            int body_jump = emit_jump(compiler, OP_JUMP);

            //update
            int update_start = compiler->function->chunk.count;
            if (fo->update) {
                struct Type* up;
                if (compile_node(compiler, fo->update, ret_types, &up) == RESULT_FAILED) return RESULT_FAILED;
            }
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->function->chunk.count + 3 - condition_start);

            //body
            patch_jump(compiler, body_jump);
            emit_byte(compiler, OP_POP); //pop condition if true
            struct Type* then_type;
            if (compile_node(compiler, fo->then_block, ret_types, &then_type) == RESULT_FAILED) return RESULT_FAILED;
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->function->chunk.count + 3 - update_start);

            patch_jump(compiler, exit_jump);
            emit_byte(compiler, OP_POP); //pop condition if false

            end_scope(compiler);
            *node_type = make_nil_type();
            return RESULT_SUCCESS;
        }
        case NODE_RETURN: {
            Return* ret = (Return*)node;
            struct Type* type;
            if (compile_node(compiler, ret->right, ret_types, &type) == RESULT_FAILED) return RESULT_FAILED;

            emit_byte(compiler, OP_RETURN);
            add_type(ret_types, type);
            *node_type = type;
            return RESULT_SUCCESS;
        }
        //expressions
        case NODE_LITERAL:      return compile_literal(compiler, node, node_type);
        case NODE_BINARY:       return compile_binary(compiler, node, node_type);
        case NODE_LOGICAL:      return compile_logical(compiler, node, node_type);
        case NODE_UNARY:        return compile_unary(compiler, node, node_type);
        case NODE_GET_PROP: {
            GetProp* gp = (GetProp*)node;
            struct Type* type_inst;
            if (compile_node(compiler, gp->inst, ret_types, &type_inst) == RESULT_FAILED) return RESULT_FAILED;

            if (type_inst->type == TYPE_LIST) {
                if (gp->prop.length == 4 && memcmp(gp->prop.start, "size", gp->prop.length) == 0) {
                    emit_byte(compiler, OP_GET_SIZE);
                    *node_type = make_int_type();
                    return RESULT_SUCCESS;
                }
                add_error(compiler, gp->prop, "Property doesn't exist on Lists.");
                return RESULT_FAILED;
            }

            if (type_inst->type == TYPE_ENUM) {
                //should be Token enum on the stack...
                emit_byte(compiler, OP_GET_PROP);
                struct ObjString* name = make_string(gp->prop.start, gp->prop.length);
                push_root(to_string(name));
                emit_short(compiler, add_constant(compiler, to_string(name))); 
                pop_root();

                Value type_val;
                if (!get_from_table(&((struct TypeEnum*)type_inst)->props, name, &type_val)) {
                    add_error(compiler, gp->prop, "Constant doesn't exist in enum.");
                    return RESULT_FAILED;
                }

                *node_type = type_inst;
                return RESULT_SUCCESS;
            }

            if (type_inst->type == TYPE_STRING) {
                if (gp->prop.length == 4 && memcmp(gp->prop.start, "size", gp->prop.length) == 0) {
                    emit_byte(compiler, OP_GET_SIZE);
                    *node_type = make_int_type();
                    return RESULT_SUCCESS;
                }
                add_error(compiler, gp->prop, "Property doesn't exist on strings.");
                return RESULT_FAILED;
            }

            if (type_inst->type == TYPE_MAP) {
                if (gp->prop.length == 4 && memcmp(gp->prop.start, "keys", gp->prop.length) == 0) {
                    emit_byte(compiler, OP_GET_KEYS);
                    *node_type = make_list_type(make_string_type());
                    return RESULT_SUCCESS;
                }
                if (gp->prop.length == 6 && memcmp(gp->prop.start, "values", gp->prop.length) == 0) {
                    emit_byte(compiler, OP_GET_VALUES);
                    *node_type = make_list_type(((struct TypeMap*)type_inst)->type);
                    return RESULT_SUCCESS;
                }
                add_error(compiler, gp->prop, "Property doesn't exist on Map.");
                return RESULT_FAILED;
            }

            if (type_inst->type == TYPE_IDENTIFIER) {
                type_inst = resolve_type(compiler, ((struct TypeIdentifier*)type_inst)->identifier);
            }

            emit_byte(compiler, OP_GET_PROP);
            struct ObjString* name = make_string(gp->prop.start, gp->prop.length);
            push_root(to_string(name));
            emit_short(compiler, add_constant(compiler, to_string(name))); 
            pop_root();

            Value type_val;
            if (!get_from_table(&((struct TypeClass*)type_inst)->props, name, &type_val)) {
                add_error(compiler, gp->prop, "Property doesn't exist on struct.");
                return RESULT_FAILED;
            }

            *node_type = type_val.as.type_type;
            return RESULT_SUCCESS;
        }
        case NODE_SET_PROP: {
            SetProp* sp = (SetProp*)node;
            struct Type* right_type;
            if (compile_node(compiler, sp->right, ret_types, &right_type) == RESULT_FAILED) return RESULT_FAILED;
            struct Type* type_inst;
            if (compile_node(compiler, ((GetProp*)(sp->inst))->inst, ret_types, &type_inst) == RESULT_FAILED) return RESULT_FAILED;
            Token prop = ((GetProp*)sp->inst)->prop;

            if (type_inst->type == TYPE_LIST) {
                if (prop.length == 4 && memcmp(prop.start, "size", prop.length) == 0) {
                    emit_byte(compiler, OP_SET_SIZE);
                    *node_type = make_int_type();
                    return RESULT_SUCCESS;
                }
                add_error(compiler, prop, "Property doesn't exist on Lists.");
                return RESULT_FAILED;
            }

            if (type_inst->type == TYPE_STRING) {
                if (prop.length == 4 && memcmp(prop.start, "size", prop.length) == 0) {
                    emit_byte(compiler, OP_SET_SIZE);
                    *node_type = make_int_type();
                    return RESULT_SUCCESS;
                }
                add_error(compiler, prop, "Property doesn't exist on strings.");
                return RESULT_FAILED;
            }

            if (type_inst->type == TYPE_IDENTIFIER) {
                type_inst = resolve_type(compiler, ((struct TypeIdentifier*)type_inst)->identifier);
            }

            if (type_inst->type == TYPE_CLASS) {
                type_inst = resolve_type(compiler, ((struct TypeClass*)type_inst)->klass);
            }

            struct ObjString* name = make_string(prop.start, prop.length);
            push_root(to_string(name));
            emit_byte(compiler, OP_SET_PROP);
            emit_short(compiler, add_constant(compiler, to_string(name))); 
            pop_root();

            Value type_val = to_nil();
            get_from_table(&((struct TypeClass*)type_inst)->props, name, &type_val);

            //TODO: should find a way to resolve identifiers immediately when possible instead of right before checking
            if (right_type->type == TYPE_IDENTIFIER) {
                right_type = resolve_type(compiler, ((struct TypeIdentifier*)right_type)->identifier);
            }

            if (!same_type(type_val.as.type_type, right_type)) {
                add_error(compiler, prop, "Property and assignment types must match.");
                return RESULT_FAILED;
            }

            *node_type = right_type;
            return RESULT_SUCCESS;
        }
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;

            //List/Map creation
            if (gv->template_type != NULL) {
                *node_type = gv->template_type;
                return RESULT_SUCCESS;
            }

            int idx = resolve_local(compiler, gv->name);
            if (idx != -1) {
                emit_byte(compiler, OP_GET_LOCAL);
                emit_byte(compiler, idx);
                *node_type = resolve_type(compiler, gv->name);
                return RESULT_SUCCESS;
            }

            int upvalue_idx = resolve_upvalue(compiler, gv->name);
            if (upvalue_idx != -1) {
                emit_byte(compiler, OP_GET_UPVALUE);
                emit_byte(compiler, upvalue_idx);
                *node_type = resolve_type(compiler, gv->name);
                return RESULT_SUCCESS;
            }

            add_error(compiler, gv->name, "Local variable not declared.");
            return RESULT_FAILED;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            struct Type* right_type;
            if (compile_node(compiler, sv->right, ret_types, &right_type) == RESULT_FAILED) return RESULT_FAILED;
            if (right_type->type == TYPE_IDENTIFIER) {
                right_type = resolve_type(compiler, ((struct TypeIdentifier*)right_type)->identifier);
            }


            Token var = ((GetVar*)(sv->left))->name;
            struct Type* var_type = resolve_type(compiler, var);
            *node_type = var_type;

            if (var_type->type == TYPE_IDENTIFIER) {
                var_type = resolve_type(compiler, ((struct TypeIdentifier*)var_type)->identifier);
            }

            int idx = resolve_local(compiler, var);
            if (idx != -1) {
                if (!same_type(var_type, right_type)) {
                    add_error(compiler, var, "Right side type must match variable type.");
                    return RESULT_FAILED;
                }

                emit_byte(compiler, OP_SET_LOCAL);
                emit_byte(compiler, idx);
                return RESULT_SUCCESS;
            }

            int upvalue_idx = resolve_upvalue(compiler, var);
            if (upvalue_idx != -1) {
                if (!same_type(var_type, right_type)) {
                    add_error(compiler, var, "Right side type must match variable type.");
                    return RESULT_FAILED;
                }

                emit_byte(compiler, OP_SET_UPVALUE);
                emit_byte(compiler, upvalue_idx);
                return RESULT_SUCCESS;
            }

            add_error(compiler, var, "Local variable not declared.");
            return RESULT_FAILED;
        }
        case NODE_GET_ELEMENT: {
            GetElement* get_idx = (GetElement*)node;
            struct Type* left_type;
            if (compile_node(compiler, get_idx->left, ret_types, &left_type) == RESULT_FAILED) return RESULT_FAILED;
            struct Type* idx_type;
            if (compile_node(compiler, get_idx->idx, ret_types, &idx_type) == RESULT_FAILED) return RESULT_FAILED;

            if (left_type->type == TYPE_STRING) {
                if (idx_type->type != TYPE_INT) {
                    add_error(compiler, get_idx->name, "Index must be integer type.");
                    return RESULT_FAILED;
                }

                emit_byte(compiler, OP_GET_ELEMENT);
                *node_type = left_type;
                return RESULT_SUCCESS;
            }

            if (left_type->type == TYPE_LIST) {
                if (idx_type->type != TYPE_INT) {
                    add_error(compiler, get_idx->name, "Index must be integer type.");
                    return RESULT_FAILED;
                }

                emit_byte(compiler, OP_GET_ELEMENT);
                *node_type = ((struct TypeList*)left_type)->type;
                return RESULT_SUCCESS;
            }

            if (left_type->type == TYPE_MAP) {
                if (idx_type->type != TYPE_STRING) {
                    add_error(compiler, get_idx->name, "Key must be string type.");
                    return RESULT_FAILED;
                }

                emit_byte(compiler, OP_GET_ELEMENT);
                *node_type = ((struct TypeList*)left_type)->type;
                return RESULT_SUCCESS;
            }

            add_error(compiler, get_idx->name, "[] access must be used on a list or map type.");
            return RESULT_FAILED;
        }
        case NODE_SET_ELEMENT: {
            SetElement* set_idx = (SetElement*)node;
            GetElement* get_idx = (GetElement*)(set_idx->left);

            struct Type* right_type;
            if (compile_node(compiler, set_idx->right, ret_types, &right_type) == RESULT_FAILED) return RESULT_FAILED;
            struct Type* left_type;
            if (compile_node(compiler, get_idx->left, ret_types, &left_type) == RESULT_FAILED) return RESULT_FAILED;
            struct Type* idx_type;
            if (compile_node(compiler, get_idx->idx, ret_types, &idx_type) == RESULT_FAILED) return RESULT_FAILED;

            if (left_type->type == TYPE_STRING) {
                if (idx_type->type != VAL_INT) {
                    add_error(compiler, get_idx->name, "Index must be integer type.");
                    return RESULT_FAILED;
                }

                if (right_type->type != TYPE_STRING) {
                    add_error(compiler, get_idx->name, "Right side type must be a string type.");
                    return RESULT_FAILED;
                }

                emit_byte(compiler, OP_SET_ELEMENT);
                *node_type = right_type;
                return RESULT_SUCCESS;
            } 

            if (left_type->type == TYPE_LIST) {
                if (idx_type->type != TYPE_INT) {
                    add_error(compiler, get_idx->name, "Index must be integer type.");
                    return RESULT_FAILED;
                }

                struct Type* template = ((struct TypeList*)left_type)->type;
                if (template->type == TYPE_IDENTIFIER) {
                    template = resolve_type(compiler, ((struct TypeIdentifier*)template)->identifier);
                }
                if (!same_type(template, right_type)) {
                    add_error(compiler, get_idx->name, "List type and right side type must match.");
                    return RESULT_FAILED;
                }

                emit_byte(compiler, OP_SET_ELEMENT);
                *node_type = right_type;
                return RESULT_SUCCESS;
            }

            if (left_type->type == TYPE_MAP) {
                if (idx_type->type != TYPE_STRING) {
                    add_error(compiler, get_idx->name, "Key must be string type.");
                    return RESULT_FAILED;
                }

                struct Type* template = ((struct TypeList*)left_type)->type;
                if (template->type == TYPE_IDENTIFIER) {
                    template = resolve_type(compiler, ((struct TypeIdentifier*)template)->identifier);
                }
                if (!same_type(template, right_type)) {
                    add_error(compiler, get_idx->name, "Map type and right side type must match.");
                    return RESULT_FAILED;
                }

                emit_byte(compiler, OP_SET_ELEMENT);
                *node_type = right_type;
                return RESULT_SUCCESS;
            }

            add_error(compiler, get_idx->name, "[] access must be used on a list or map type.");
            return RESULT_FAILED;
        }
        case NODE_CALL: {
            Call* call = (Call*)node;

            struct Type* type;
            if (compile_node(compiler, call->left, ret_types, &type) == RESULT_FAILED) return RESULT_FAILED;

            if (type->type == TYPE_LIST) {
                struct TypeList* type_list = (struct TypeList*)type;
                if (call->arguments->count != 0) {
                    add_error(compiler, call->name, "List constructor must have 1 argument for default value.");
                    return RESULT_FAILED;
                }

                struct Type* list_template = type_list->type;
                if (list_template->type == TYPE_IDENTIFIER) {
                    list_template = resolve_type(compiler, ((struct TypeIdentifier*)list_template)->identifier);
                }

                emit_byte(compiler, OP_LIST);
                *node_type = type;
                return RESULT_SUCCESS;
            }

            if (type->type == TYPE_MAP) {
                struct TypeMap* type_map = (struct TypeMap*)type;
                if (call->arguments->count != 0) {
                    add_error(compiler, call->name, "Map constructor must have 1 argument for default value.");
                    return RESULT_FAILED;
                }

                struct Type* map_template = type_map->type;
                if (map_template->type == TYPE_IDENTIFIER) {
                    map_template = resolve_type(compiler, ((struct TypeIdentifier*)map_template)->identifier);
                }

                emit_byte(compiler, OP_MAP);
                *node_type = type;
                return RESULT_SUCCESS;
            }

            if (type->type == TYPE_CLASS) {
                struct TypeClass* type_class = (struct TypeClass*)type;
                emit_byte(compiler, OP_INSTANCE);
                *node_type = type;
                return RESULT_SUCCESS;
            }

            if (type->type != TYPE_FUN) {
                add_error(compiler, call->name, "Calls must be used on a function type.");
                return RESULT_FAILED;
            }

            //otherwise it's TYPE_FUN

            struct TypeFun* type_fun = (struct TypeFun*)type;
            struct TypeArray* params = (struct TypeArray*)(type_fun->params);

            if (call->arguments->count != params->count) {
                add_error(compiler, call->name, "Argument count must match function parameter count.");
                return RESULT_FAILED;
            }

            int min = call->arguments->count < params->count ? call->arguments->count : params->count; //Why is this needed?
            for (int i = 0; i < min; i++) {
                struct Type* arg_type;
                if (compile_node(compiler, call->arguments->nodes[i], ret_types, &arg_type) == RESULT_FAILED) return RESULT_FAILED;

                if (arg_type->type == TYPE_IDENTIFIER) {
                    arg_type = resolve_type(compiler, ((struct TypeIdentifier*)arg_type)->identifier);
                }

                struct Type* param_type = params->types[i];
                if (param_type->type == TYPE_IDENTIFIER) {
                    param_type = resolve_type(compiler, ((struct TypeIdentifier*)param_type)->identifier);
                }

                if (!same_type(param_type, arg_type)) {
                    add_error(compiler, call->name, "Argument type must match parameter type.");
                    return RESULT_FAILED;
                }
            }

            emit_byte(compiler, OP_CALL);
            emit_byte(compiler, (uint8_t)(call->arguments->count));

            *node_type = type_fun->ret;
            return RESULT_SUCCESS;
        }
        case NODE_NIL: {
            Nil* nil = (Nil*)node;
            emit_byte(compiler, OP_NIL);
            *node_type =  make_nil_type();
            return RESULT_SUCCESS;
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
    compiler->types = NULL;
    compiler->nodes = NULL;

    compiler->enclosing = current_compiler;
    current_compiler = compiler;
}

void free_compiler(struct Compiler* compiler) {
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
    current_compiler = compiler->enclosing;
    pop_root();
    pop_root();
}

static ResultCode compile_function(struct Compiler* compiler, struct NodeList* nl, struct TypeArray** type_array) {
    struct Type* ret_types = make_array_type();
    ResultCode ret_result = RESULT_SUCCESS;
    for (int i = 0; i < nl->count; i++) {
        struct Type* type;
        ResultCode result = compile_node(compiler, nl->nodes[i], (struct TypeArray*)ret_types, &type);
        if (result == RESULT_FAILED) ret_result = result;
    }

    if (((struct TypeArray*)ret_types)->count == 0) {
        emit_byte(compiler, OP_NIL);
        emit_byte(compiler, OP_RETURN);
    }

    *type_array = (struct TypeArray*)ret_types;

    return ret_result;
}

static Value string_native(int arg_count, Value* args) {
    switch(args[0].type) {
        case VAL_INT: {
            int num = args[0].as.integer_type;

            char* str = ALLOCATE_ARRAY(char);
            str = GROW_ARRAY(str, char, 80, 0);
            int len = sprintf(str, "%d", num);
            str = GROW_ARRAY(str, char, len + 1, 80);

            return to_string(take_string(str, len));
        }
        case VAL_FLOAT: {
            double num = args[0].as.float_type;

            char* str = ALLOCATE_ARRAY(char);
            str = GROW_ARRAY(str, char, 80, 0);
            int len = sprintf(str, "%f", num);
            str = GROW_ARRAY(str, char, len + 1, 80);

            return to_string(take_string(str, len));
        }
        case VAL_BOOL:
            if (args[0].as.boolean_type) {
                return to_string(make_string("true", 4));
            }
            return to_string(make_string("false", 5));
        case VAL_NIL:
            return to_string(make_string("nil", 3));
    }
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
        case VAL_NIL:
            printf("nil\n");
            break;
    }
    return to_nil();
}

static struct Type* define_native(struct Compiler* compiler, const char* name, Value (*function)(int, Value*), struct Type* type) {
    add_local(compiler, make_artificial_token(name), type);
    emit_byte(compiler, OP_NATIVE);
    Value native = to_native(make_native(function));
    push_root(native);
    emit_short(compiler, add_constant(compiler, native));
    pop_root();
}

static void define_clock(struct Compiler* compiler) {
    define_native(compiler, "clock", clock_native, make_fun_type(make_array_type(), make_float_type()));
}

static void define_print(struct Compiler* compiler) {
    struct Type* str_type = make_string_type();
    struct Type* int_type = make_int_type();
    struct Type* float_type = make_float_type();
    struct Type* nil_type = make_nil_type();
    //using dummy token with enum type to allow printing enums (as integer)
    //otherwise the typechecker sees it as an error
    struct Type* enum_type = make_enum_type(make_dummy_token());
    str_type->opt = int_type;
    int_type->opt = float_type;
    float_type->opt = nil_type;
    nil_type->opt = enum_type;
    struct Type* sl = make_array_type();
    add_type((struct TypeArray*)sl, str_type);
    define_native(compiler, "print", print_native, make_fun_type((struct Type*)sl, make_nil_type()));
}

static void define_string(struct Compiler* compiler) {
    struct Type* int_type = make_int_type();
    struct Type* float_type = make_float_type();
    struct Type* bool_type = make_bool_type();
    struct Type* nil_type = make_nil_type();
    int_type->opt = float_type;
    float_type->opt = bool_type;
    bool_type->opt = nil_type;
    struct Type* sl = make_array_type();
    add_type((struct TypeArray*)sl, int_type);
    define_native(compiler, "string", string_native, make_fun_type((struct Type*)sl, make_string_type()));
}

ResultCode compile_script(struct Compiler* compiler, struct NodeList* nl) {
    define_clock(compiler);
    define_print(compiler);
    define_string(compiler);

    struct TypeArray* ret_types;
    ResultCode result = compile_function(compiler, nl, &ret_types);

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

