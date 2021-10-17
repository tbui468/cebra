#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "compiler.h"
#include "memory.h"
#include "obj.h"

#define COMPILE_NODE(node, ret_node, node_type) \
            if (compile_node(compiler, node, ret_node, node_type) == RESULT_FAILED) return RESULT_FAILED

#define CHECK_TYPE(fail_condition, token, msg) \
                if (fail_condition) { \
                    add_error(compiler, token, msg); \
                    return RESULT_FAILED; \
                }

struct Compiler* current_compiler = NULL;
struct Compiler* script_compiler = NULL;
static ResultCode compile_node(struct Compiler* compiler, struct Node* node, struct TypeArray* ret_types, struct Type** node_type);
static ResultCode compile_function(struct Compiler* compiler, struct NodeList* nl, struct TypeArray** type_array);

static bool struct_or_function_to_nil(struct Type* var_type, struct Type* right_type) {
    if (right_type->type != TYPE_NIL) return false;
    if (var_type->type != TYPE_FUN && var_type->type != TYPE_STRUCT) return false;
    return true;
}

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
    COMPILE_NODE(unary->right, NULL, &type);
    emit_byte(compiler, OP_NEGATE);
    *node_type = type;
    return RESULT_SUCCESS;
}

static ResultCode compile_logical(struct Compiler* compiler, struct Node* node, struct Type** node_type) {
    Logical* logical = (Logical*)node;

    if (logical->name.type == TOKEN_AND) {
        struct Type* left_type;
        COMPILE_NODE(logical->left, NULL, &left_type);
        int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
        struct Type* right_type;
        COMPILE_NODE(logical->right, NULL, &right_type);
        patch_jump(compiler, false_jump);
        CHECK_TYPE(!same_type(left_type, right_type), logical->name, "Left and right types must match.");
        *node_type = make_bool_type();
        return RESULT_SUCCESS;
    }

    if (logical->name.type == TOKEN_OR) {
        struct Type* left_type;
        COMPILE_NODE(logical->left, NULL, &left_type);
        int true_jump = emit_jump(compiler, OP_JUMP_IF_TRUE);
        emit_byte(compiler, OP_POP);
        struct Type* right_type;
        COMPILE_NODE(logical->right, NULL, &right_type);
        patch_jump(compiler, true_jump);
        CHECK_TYPE(!same_type(left_type, right_type), logical->name, "Left and right types must match.");
        *node_type = make_bool_type();
        return RESULT_SUCCESS;
    }

    if (logical->name.type == TOKEN_IN) {
        struct Type* element_type;
        COMPILE_NODE(logical->left, NULL, &element_type);
        struct Type* list_type;
        COMPILE_NODE(logical->right, NULL, &list_type);

        CHECK_TYPE(list_type->type != TYPE_LIST, logical->name, "Identifier after 'in' must reference a List.");

        CHECK_TYPE(!same_type(element_type, ((struct TypeList*)list_type)->type), logical->name, "Type left of 'in' must match List element type.");

        emit_byte(compiler, OP_IN_LIST);
        *node_type = make_bool_type();
        return RESULT_SUCCESS;
    }

    struct Type* left_type;
    COMPILE_NODE(logical->left, NULL, &left_type);
    struct Type* right_type;
    COMPILE_NODE(logical->right, NULL, &right_type);

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
    COMPILE_NODE(binary->left, NULL, &type1);
    struct Type* type2;
    COMPILE_NODE(binary->right, NULL, &type2);
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
    if (get_from_table(&script_compiler->globals, str, &val)) {
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

static ResultCode compile_decl_var(struct Compiler* compiler, DeclVar* dv, struct TypeArray* ret_types, struct Type** node_type) {
    struct Type* type = NULL;

    if (declared_in_scope(compiler, dv->name)) {
        add_error(compiler, dv->name, "Identifier already defined.");
        return RESULT_FAILED;
    }

    //inferred type, defined
    if (dv->type->type == TYPE_INFER) {
        int idx = add_local(compiler, dv->name, dv->type);

        COMPILE_NODE(dv->right, NULL, &type);
        CHECK_TYPE(type->type == TYPE_NIL , dv->name,
                   "Inferred type cannot be assigned to 'nil.");

        CHECK_TYPE(type->type == TYPE_DECL, dv->name, 
                   "Variable cannot be assigned to a user declared type.");

        set_local(compiler, dv->name, type, idx);
    }
    
    //explicit type, defined and parameters
    if (dv->type->type != TYPE_INFER) {
        int idx = add_local(compiler, dv->name, dv->type);
        COMPILE_NODE(dv->right, NULL, &type);

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

static ResultCode compile_node(struct Compiler* compiler, struct Node* node, struct TypeArray* ret_types, struct Type** node_type) {
    if (node == NULL) {
        *node_type = make_nil_type();
        return RESULT_SUCCESS;
    }
    switch(node->type) {
        //declarations
        case NODE_DECL_VAR: {
            ResultCode result = compile_decl_var(compiler, (DeclVar*)node, ret_types, node_type);
            *node_type = make_nil_type();
            return result;
        }
        case NODE_SEQUENCE: {
                                /*
            struct Sequence* seq = (struct Sequence*)node;

            //compile right side left-to-right
            struct Type* right_seq_type;
            if (seq->right->type == NODE_SEQUENCE) {
                COMPILE_NODE(seq->right, NULL, &right_seq_type);
            } else if (seq->right->type == NODE_LIST) {
                struct NodeList* nl = (struct NodeList*)(seq->right);
                struct TypeArray ta = make_type_array();
                for (int i = 0; i < nl->count; i++) {
                    struct Type* var_type;
                    COMPILE_NODE(nl->nodes[i], NULL, &var_type);
                    add_type(ta, var_type);
                }
                right_seq_type = (struct Type*)ta;
            }

            //x, y, z = 1, 2, 3
            //[1][2][3]
            //compile z, y and then x manually to set to stack top
            //compile left side right-to-left to align with values pushed onto stack
            //whe compiling right side above
            for (int i = seq->left->count - 1; i >= 0; i--) {
                struct Type* left_type;
                COMPILE_NODE(nl->nodes[i], NULL, &left_type);
            }

            //compile right side
                //if sequence, just compile entire node
                //if NodeList, compile from left to right
            //compile left side from right to left order
                //need to check and emit proper bytes for OP_SET_[VAR|ELEMENT|PROP]
                //good chance to pull out reusable code there to insert here
            if (!same_type(left_seq_type, right_seq_type)) {
                add_error(compiler, seq->name, "Sequence types must all match.");
                return RESULT_FAILED;
            }
            *node_type = left_seq_type; //== right_seq_type
            return RESULT_SUCCESS;*/
        }
        case NODE_FUN: {
            DeclFun* df = (DeclFun*)node;

            if (df->anonymous) {
                int set_idx = 0;
                if (df->name.length != 0) {
                    if (declared_in_scope(compiler, df->name)) {
                        add_error(compiler, df->name, "Identifier already defined.");
                        return RESULT_FAILED;
                    }

                    set_idx = add_local(compiler, df->name, make_infer_type());
                }

                //check for invalid parameter types
                struct TypeFun* fun_type = (struct TypeFun*)(df->type);
                for (int i = 0; i < fun_type->params->count; i++) {
                    if (fun_type->params->types[i]->type == TYPE_NIL) {
                        add_error(compiler, df->name, "Parameter identifier type invalid.");
                        return RESULT_FAILED;
                    }
                }

                //compile function using new compiler
                struct Compiler func_comp;
                init_compiler(&func_comp, df->name.start, df->name.length, df->parameters->count, df->name, df->type);
                add_node(df->parameters, df->body);
                struct TypeArray* inner_ret_types;
                ResultCode result = compile_function(&func_comp, df->parameters, &inner_ret_types); 
                if (result == RESULT_FAILED) {
                    free_compiler(&func_comp);
                    return result;
                }

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(func_comp.function);
    int i = 0;
    printf("-------------------\n");
    while (i < func_comp.function->chunk.count) {
       OpCode op = func_comp.function->chunk.codes[i++];
       printf("[ %s ]\n", op_to_string(op));
    }
#endif


                struct TypeFun* typefun = (struct TypeFun*)df->type;
                if (inner_ret_types->count == 0 &&
                    typefun->returns->count != 1 &&
                    typefun->returns->types[0]->type != TYPE_NIL)
                {
                    add_error(compiler, df->name, "A function with no return must declared with no return type.");
                    free_compiler(&func_comp);
                    return RESULT_FAILED;
                }

                //@only grabbin first return for now
                for (int i = 0; i < inner_ret_types->count; i++) {
                    struct Type* inner_type = inner_ret_types->types[i];
                    if (!same_type(typefun->returns->types[0], inner_type)) {
                        add_error(compiler, df->name, "Return type must match type in function declaration.");
                        free_compiler(&func_comp);
                        return RESULT_FAILED;
                    }
                }

                emit_byte(compiler, OP_FUN);
                emit_short(compiler, add_constant(compiler, to_function(func_comp.function)));
                emit_byte(compiler, func_comp.upvalue_count);
                for (int i = 0; i < func_comp.upvalue_count; i++) {
                    emit_byte(compiler, func_comp.upvalues[i].is_local);
                    emit_byte(compiler, func_comp.upvalues[i].index);
                }

                free_compiler(&func_comp);

                *node_type = df->type;
                return RESULT_SUCCESS;
            } else {
                //compile function into new compiler
                struct Compiler func_comp;
                init_compiler(&func_comp, df->name.start, df->name.length, df->parameters->count, df->name, df->type);
                add_node(df->parameters, df->body);
                struct TypeArray* inner_ret_types;
                ResultCode result = compile_function(&func_comp, df->parameters, &inner_ret_types); 
                if (result == RESULT_FAILED) {
                    free_compiler(&func_comp);
                    return result;
                }

#ifdef DEBUG_DISASSEMBLE
    disassemble_chunk(func_comp.function);
    int i = 0;
    printf("-------------------\n");
    while (i < func_comp.function->chunk.count) {
       OpCode op = func_comp.function->chunk.codes[i++];
       printf("[ %s ]\n", op_to_string(op));
    }
#endif


                struct TypeFun* typefun = (struct TypeFun*)df->type;
                if (inner_ret_types->count == 0 &&
                    typefun->returns->count != 1 &&
                    typefun->returns->types[0]->type != TYPE_NIL)
                {
                    add_error(compiler, df->name, "A function with no return must be declared with no return type.");
                    free_compiler(&func_comp);
                    return RESULT_FAILED;
                }

                for (int i = 0; i < inner_ret_types->count; i++) {
                    struct Type* inner_type = inner_ret_types->types[i];
                    //@ only grabbing first return for now
                    if (!same_type(typefun->returns->types[0], inner_type)) {
                        add_error(compiler, df->name, "Return type must match type in function declaration.");
                        free_compiler(&func_comp);
                        return RESULT_FAILED;
                    }
                }

                if (func_comp.upvalue_count > 0) {
                    add_error(compiler, df->name, "Functions cannot capture values.  To create closures, create and anonymous function.");
                    free_compiler(&func_comp);
                    return RESULT_FAILED;
                }

                emit_byte(compiler, OP_FUN);
                emit_short(compiler, add_constant(compiler, to_function(func_comp.function)));
                emit_byte(compiler, func_comp.upvalue_count);

                free_compiler(&func_comp);

                emit_byte(compiler, OP_ADD_GLOBAL);

                *node_type = df->type;
                return RESULT_SUCCESS;
            }
        }
        case NODE_STRUCT: {
            struct DeclStruct* dc = (struct DeclStruct*)node;

            //compile super so that it's on the stack
            struct Type* super_type = NULL;
            if (dc->super != NULL) {
                COMPILE_NODE(dc->super, ret_types, &super_type);
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
            get_from_table(&compiler->globals, struct_string, &v);
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
                struct Type* right_type;
                if (compile_decl_var(compiler, (DeclVar*)node, NULL, &right_type) == RESULT_FAILED) {
                    return RESULT_FAILED;
                }

                //update types in TypeStruct if property type is inferred
                Value v;
                get_from_table(&klass_type->props, prop_name, &v);
                if (v.as.type_type->type == TYPE_INFER) {
                    set_table(&klass_type->props, prop_name, to_type(right_type));
                }

                Value left_val_type;
                get_from_table(&klass_type->props, prop_name, &left_val_type);
                CHECK_TYPE(!same_type(left_val_type.as.type_type, right_type) && 
                           !struct_or_function_to_nil(left_val_type.as.type_type, right_type), 
                           dv->name, "Property type must match right hand side.");

                emit_byte(compiler, OP_ADD_PROP);
                emit_short(compiler, add_constant(compiler, to_string(prop_name)));
                end_scope(compiler);

                pop_root();
            }

            //set globals in vm
            emit_byte(compiler, OP_ADD_GLOBAL);

            *node_type = (struct Type*)klass_type;

            return RESULT_SUCCESS;
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
                set_table(&obj_enum->props, prop_name, to_integer(i));
                pop_root();
            }

            emit_byte(compiler, OP_ENUM);
            emit_short(compiler, add_constant(compiler, to_enum(obj_enum)));

            //set globals in vm
            emit_byte(compiler, OP_ADD_GLOBAL);

            //Get type already completely defined in parser
            Value v;
            get_from_table(&compiler->globals, obj_enum->name, &v);
            *node_type = v.as.type_type;

            pop_root(); //struct ObjEnum* 'obj_enum'
            return RESULT_SUCCESS;
        }
        //statements
        case NODE_EXPR_STMT: {
            ExprStmt* es = (ExprStmt*)node;
            struct Type* type;
            COMPILE_NODE(es->expr, ret_types, &type);

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
                COMPILE_NODE(block->decl_list->nodes[i], ret_types, &type);
            }
            end_scope(compiler);
            *node_type = make_nil_type();
            return RESULT_SUCCESS;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            struct Type* cond;
            COMPILE_NODE(ie->condition, ret_types, &cond);

            CHECK_TYPE(cond->type != TYPE_BOOL, ie->name, 
                       "Condition must evaluate to boolean.");

            int jump_then = emit_jump(compiler, OP_JUMP_IF_FALSE); 

            emit_byte(compiler, OP_POP);
            struct Type* then_type;
            COMPILE_NODE(ie->then_block, ret_types, &then_type);
            int jump_else = emit_jump(compiler, OP_JUMP);

            patch_jump(compiler, jump_then);
            emit_byte(compiler, OP_POP);

            struct Type* else_type;
            COMPILE_NODE(ie->else_block, ret_types, &else_type);
            patch_jump(compiler, jump_else);

            *node_type = make_nil_type();
            return RESULT_SUCCESS;
        }
        case NODE_WHEN: {
            struct When* when = (struct When*)node;
            int jump_ends[256];
            int je_count = 0;
            for (int i = 0; i < when->cases->count; i++) {
                IfElse* kase = (IfElse*)(when->cases->nodes[i]);
                struct Type* cond_type;
                COMPILE_NODE(kase->condition, ret_types, &cond_type);
                CHECK_TYPE(cond_type->type != TYPE_BOOL, kase->name, 
                        "The expressions after 'when' and 'is' must be comparable using a '==' operator.");

                int jump_then = emit_jump(compiler, OP_JUMP_IF_FALSE);
                emit_byte(compiler, OP_POP);
                struct Type* then_type;
                COMPILE_NODE(kase->then_block, ret_types, &then_type);
                int jump_end = emit_jump(compiler, OP_JUMP);
                if (je_count >= 255) {
                    add_error(compiler, when->name, "Limit of 256 'is' and 'else' cases in a 'when' statement.");
                    return RESULT_FAILED;
                }
                jump_ends[je_count] = jump_end; 
                je_count++;

                patch_jump(compiler, jump_then);
                emit_byte(compiler, OP_POP);
            }

            for (int i = 0; i < je_count; i++) {
                patch_jump(compiler, jump_ends[i]);
            }

            *node_type = make_nil_type();
            return RESULT_SUCCESS;
        }
        case NODE_WHILE: {
            While* wh = (While*)node;
            int start = compiler->function->chunk.count;
            struct Type* cond;
            COMPILE_NODE(wh->condition, ret_types, &cond);
            CHECK_TYPE(cond->type != TYPE_BOOL, wh->name,
                       "Condition must evaluate to boolean.");

            int false_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);

            emit_byte(compiler, OP_POP);
            struct Type* then_block;
            COMPILE_NODE(wh->then_block, ret_types, &then_block);

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
            COMPILE_NODE(fo->initializer, ret_types, &init);

            //condition
            int condition_start = compiler->function->chunk.count;
            struct Type* cond;
            COMPILE_NODE(fo->condition, ret_types, &cond);
            CHECK_TYPE(cond->type != TYPE_BOOL, fo->name, "Condition must evaluate to boolean.");

            int exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
            int body_jump = emit_jump(compiler, OP_JUMP);

            //update
            int update_start = compiler->function->chunk.count;
            if (fo->update) {
                struct Type* up;
                COMPILE_NODE(fo->update, ret_types, &up);
            }
            emit_jump_by(compiler, OP_JUMP_BACK, compiler->function->chunk.count + 3 - condition_start);

            //body
            patch_jump(compiler, body_jump);
            emit_byte(compiler, OP_POP); //pop condition if true
            struct Type* then_type;
            COMPILE_NODE(fo->then_block, ret_types, &then_type);
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
            COMPILE_NODE(ret->right, ret_types, &type);
            emit_byte(compiler, OP_RETURN);

            //@need to take care of multiple returns later

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
            COMPILE_NODE(gp->inst, ret_types, &type_inst);

            if (type_inst->type == TYPE_LIST) {
                if (same_token_literal(gp->prop, make_token(TOKEN_DUMMY, 0, "size", 4))) {
                    emit_byte(compiler, OP_GET_SIZE);
                    *node_type = make_int_type();
                    return RESULT_SUCCESS;
                }
                add_error(compiler, gp->prop, "Property doesn't exist on Lists.");
                return RESULT_FAILED;
            }

            if (type_inst->type == TYPE_DECL) {
                struct TypeDecl* td = (struct TypeDecl*)type_inst;
                if (td->custom_type->type == TYPE_ENUM) {
                    //should be Token enum on the stack...
                    struct TypeEnum* te = (struct TypeEnum*)(td->custom_type);
                    emit_byte(compiler, OP_GET_PROP);
                    struct ObjString* name = make_string(gp->prop.start, gp->prop.length);
                    push_root(to_string(name));
                    emit_short(compiler, add_constant(compiler, to_string(name))); 
                    pop_root();

                    Value type_val;
                    if (!get_from_table(&te->props, name, &type_val)) {
                        add_error(compiler, gp->prop, "Constant doesn't exist in enum.");
                        return RESULT_FAILED;
                    }

                    *node_type = (struct Type*)te;
                    return RESULT_SUCCESS;
                }
                return RESULT_FAILED;
            }

            if (type_inst->type == TYPE_STRING) {
                if (same_token_literal(gp->prop, make_token(TOKEN_DUMMY, 0, "size", 4))) {
                    emit_byte(compiler, OP_GET_SIZE);
                    *node_type = make_int_type();
                    return RESULT_SUCCESS;
                }
                add_error(compiler, gp->prop, "Property doesn't exist on strings.");
                return RESULT_FAILED;
            }

            if (type_inst->type == TYPE_MAP) {
                if (same_token_literal(gp->prop, make_token(TOKEN_DUMMY, 0, "keys", 4))) {
                    emit_byte(compiler, OP_GET_KEYS);
                    *node_type = make_list_type(make_string_type());
                    return RESULT_SUCCESS;
                }
                if (same_token_literal(gp->prop, make_token(TOKEN_DUMMY, 0, "values", 6))) {
                    emit_byte(compiler, OP_GET_VALUES);
                    *node_type = make_list_type(((struct TypeMap*)type_inst)->type);
                    return RESULT_SUCCESS;
                }
                add_error(compiler, gp->prop, "Property doesn't exist on Map.");
                return RESULT_FAILED;
            }

            emit_byte(compiler, OP_GET_PROP);
            struct ObjString* name = make_string(gp->prop.start, gp->prop.length);
            push_root(to_string(name));
            emit_short(compiler, add_constant(compiler, to_string(name))); 
            pop_root();

            Value type_val = to_nil();
            struct Type* current = type_inst;
            while (current != NULL) {
                struct TypeStruct* tc = (struct TypeStruct*)current;
                if (get_from_table(&tc->props, name, &type_val)) break;
                current = tc->super;
            }

            if (current == NULL) {
                add_error(compiler, gp->prop, "Property not found in instance.");
                return RESULT_FAILED;
            }

            *node_type = type_val.as.type_type;
            return RESULT_SUCCESS;
        }
        case NODE_SET_PROP: {
            SetProp* sp = (SetProp*)node;
            struct Type* right_type;
            COMPILE_NODE(sp->right, ret_types, &right_type);
            struct Type* type_inst;
            COMPILE_NODE(((GetProp*)(sp->inst))->inst, ret_types, &type_inst);
            Token prop = ((GetProp*)sp->inst)->prop;

            if (type_inst->type == TYPE_LIST) {
                if (same_token_literal(prop, make_token(TOKEN_DUMMY, 0, "size", 4))) {
                    emit_byte(compiler, OP_SET_SIZE);
                    *node_type = make_int_type();
                    return RESULT_SUCCESS;
                }
                add_error(compiler, prop, "Property doesn't exist on Lists.");
                return RESULT_FAILED;
            }

            if (type_inst->type == TYPE_STRUCT) {
                struct ObjString* name = make_string(prop.start, prop.length);
                push_root(to_string(name));
                emit_byte(compiler, OP_SET_PROP);
                emit_short(compiler, add_constant(compiler, to_string(name))); 
                int depth = 0;
                emit_byte(compiler, depth);
                pop_root();

                Value type_val = to_nil();
                struct Type* current = type_inst;
                while (current != NULL) {
                    struct TypeStruct* tc = (struct TypeStruct*)current;
                    if (get_from_table(&tc->props, name, &type_val)) break;
                    current = tc->super;
                }

                if (current == NULL) {
                    add_error(compiler, prop, "Property not found in instance.");
                    return RESULT_FAILED;
                }

                CHECK_TYPE(!same_type(type_val.as.type_type, right_type) && 
                           !struct_or_function_to_nil(type_val.as.type_type, right_type), 
                           prop, "Property and assignment types must match.");

                *node_type = right_type;
                return RESULT_SUCCESS;
            }
            add_error(compiler, prop, "Property cannot be set.");
            return RESULT_FAILED;
        }
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;

            int idx = resolve_local(compiler, gv->name);
            OpCode op = OP_GET_LOCAL;
            if (idx == -1) {
                idx = resolve_upvalue(compiler, gv->name);
                op = OP_GET_UPVALUE;
            }

            if (idx != -1) {
                emit_byte(compiler, op);
                emit_byte(compiler, idx);
                *node_type = resolve_type(compiler, gv->name);
                return RESULT_SUCCESS;
            } 

            //check if global -this needs to check enclosing compiler too
            struct ObjString* name = make_string(gv->name.start, gv->name.length);
            push_root(to_string(name));
            Value v;
            if (get_from_table(&script_compiler->globals, name, &v)) {
                emit_byte(compiler, OP_GET_GLOBAL);
                emit_short(compiler, add_constant(compiler, to_string(name)));
                pop_root();
                if (v.as.type_type->type == TYPE_STRUCT || v.as.type_type->type == TYPE_ENUM) {
                    *node_type = make_decl_type(v.as.type_type);
                } else {
                    *node_type = v.as.type_type;
                }
                return RESULT_SUCCESS;
            }
            pop_root();
            print_token(gv->name);
            add_error(compiler, gv->name, "[GetVar] Attempting to access undeclared variable.");
            return RESULT_FAILED;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            struct Type* right_type;
            COMPILE_NODE(sv->right, ret_types, &right_type);

            Token var = ((GetVar*)(sv->left))->name;
            struct Type* var_type = resolve_type(compiler, var);
            *node_type = var_type;

            int idx = resolve_local(compiler, var);
            OpCode op = OP_SET_LOCAL;
            if (idx == -1) {
                idx = resolve_upvalue(compiler, var);
                op = OP_SET_UPVALUE;
            }

            if (idx != -1) {
                CHECK_TYPE(!same_type(var_type, right_type) && !struct_or_function_to_nil(var_type, right_type), 
                           var, "Right side type must match variable type.");

                emit_byte(compiler, op);
                emit_byte(compiler, idx);
                int depth = 0;
                emit_byte(compiler, depth);
                return RESULT_SUCCESS;
            }

            add_error(compiler, var, "Local variable not declared.");
            return RESULT_FAILED;
        }
        case NODE_GET_ELEMENT: {
            GetElement* get_idx = (GetElement*)node;
            struct Type* left_type;
            COMPILE_NODE(get_idx->left, ret_types, &left_type);
            struct Type* idx_type;
            COMPILE_NODE(get_idx->idx, ret_types, &idx_type);

            if (left_type->type == TYPE_STRING) {
                CHECK_TYPE(idx_type->type != TYPE_INT, get_idx->name, "Index must be integer type.");

                emit_byte(compiler, OP_GET_ELEMENT);
                *node_type = left_type;
                return RESULT_SUCCESS;
            }

            if (left_type->type == TYPE_LIST) {
                CHECK_TYPE(idx_type->type != TYPE_INT, get_idx->name, "Index must be integer type.");

                emit_byte(compiler, OP_GET_ELEMENT);
                *node_type = ((struct TypeList*)left_type)->type;
                return RESULT_SUCCESS;
            }

            if (left_type->type == TYPE_MAP) {
                CHECK_TYPE(idx_type->type != TYPE_STRING, get_idx->name, "Key must be string type.");

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
            COMPILE_NODE(set_idx->right, ret_types, &right_type);
            struct Type* left_type;
            COMPILE_NODE(get_idx->left, ret_types, &left_type);
            struct Type* idx_type;
            COMPILE_NODE(get_idx->idx, ret_types, &idx_type);

            if (left_type->type == TYPE_LIST) {
                CHECK_TYPE(idx_type->type != TYPE_INT, get_idx->name, "Index must be integer type.");

                struct Type* template = ((struct TypeList*)left_type)->type;
                CHECK_TYPE(!same_type(template, right_type), get_idx->name, "List type and right side type must match.");

                emit_byte(compiler, OP_SET_ELEMENT);
                *node_type = right_type;
                return RESULT_SUCCESS;
            }

            if (left_type->type == TYPE_MAP) {
                CHECK_TYPE(idx_type->type != TYPE_STRING, get_idx->name, "Key must be string type.");

                struct Type* template = ((struct TypeList*)left_type)->type;
                CHECK_TYPE(!same_type(template, right_type), get_idx->name, "Map type and right side type must match.");

                emit_byte(compiler, OP_SET_ELEMENT);
                *node_type = right_type;
                return RESULT_SUCCESS;
            }

            add_error(compiler, get_idx->name, "[] access must be used on a list or map type.");
            return RESULT_FAILED;
        }
        case NODE_CONTAINER: {
            struct DeclContainer* dc = (struct DeclContainer*)node;
            if (dc->type->type == TYPE_LIST) {
                emit_byte(compiler, OP_LIST);
                *node_type = dc->type;
                return RESULT_SUCCESS;
            }
            if (dc->type->type == TYPE_MAP) {
                emit_byte(compiler, OP_MAP);
                *node_type = dc->type;
                return RESULT_SUCCESS;
            }
            add_error(compiler, dc->name, "Invalid identifier for container.");
            return RESULT_FAILED;
        }
        case NODE_CALL: {
            Call* call = (Call*)node;

            struct Type* type;
            COMPILE_NODE(call->left, ret_types, &type);

            if (type->type == TYPE_DECL) {
                struct TypeDecl* td = (struct TypeDecl*)type;
                if (td->custom_type->type == TYPE_STRUCT) {
                    struct TypeStruct* type_struct = (struct TypeStruct*)(td->custom_type);
                    emit_byte(compiler, OP_INSTANCE);
                    *node_type = (struct Type*)type_struct;
                    return RESULT_SUCCESS;
                }
                add_error(compiler, call->name, "Calls can only be used on functions or to instantiate structs.");
                return RESULT_FAILED;
            }

            if (type->type == TYPE_FUN) {
                struct TypeFun* type_fun = (struct TypeFun*)type;
                struct TypeArray* params = (type_fun->params);

                if (call->arguments->count != params->count) {
                    add_error(compiler, call->name, "Argument count must match function parameter count.");
                    return RESULT_FAILED;
                }

                int min = call->arguments->count < params->count ? call->arguments->count : params->count; //Why is this needed?
                for (int i = 0; i < min; i++) {
                    struct Type* arg_type;
                    COMPILE_NODE(call->arguments->nodes[i], ret_types, &arg_type);

                    struct Type* param_type = params->types[i];

                    CHECK_TYPE(!same_type(param_type, arg_type), call->name, "Argument type must match parameter type.");
                }

                emit_byte(compiler, OP_CALL);
                emit_byte(compiler, (uint8_t)(call->arguments->count));

                //@need to integrate multiple returns later - just grabbing return at index 0 for now
                //*node_type = (struct Type*)(type_fun->returns);
                *node_type = ((struct TypeArray*)(type_fun->returns))->types[0];
                return RESULT_SUCCESS;
            }

            add_error(compiler, call->name, "Calls can only be made on functions, structs, List and Map.");
            return RESULT_FAILED;
        }
        case NODE_NIL: {
            Nil* nil = (Nil*)node;
            emit_byte(compiler, OP_NIL);
            *node_type =  make_nil_type();
            return RESULT_SUCCESS;
        }
        case NODE_CAST: {
            Cast* cast = (Cast*)node;
            struct Type* left;
            COMPILE_NODE(cast->left, ret_types, &left);

            if (is_primitive(left) && is_primitive(cast->type)) {
                CHECK_TYPE(left->type == TYPE_NIL && cast->type->type != TYPE_STRING, cast->name, "'nil' types can only be cast to string.");
                CHECK_TYPE(cast->type->type == TYPE_NIL, cast->name, "Cannot cast to 'nil' type.");
                CHECK_TYPE(left->type == cast->type->type, cast->name, "Cannot cast to same type.");
                emit_byte(compiler, OP_CAST);
                emit_short(compiler, cast->type->type);
                *node_type = cast->type;
                return RESULT_SUCCESS;
            }

            CHECK_TYPE(left->type != TYPE_STRUCT || cast->type->type != TYPE_STRUCT,
                    cast->name, "Type casts with 'as' must be applied to structure types or primitive types.");

            struct TypeStruct* from = (struct TypeStruct*)(left);
            struct TypeStruct* to = (struct TypeStruct*)(cast->type);

            CHECK_TYPE(same_token_literal(from->name, to->name), 
                       cast->name, "Attempting to cast to own type.");

            bool cast_up = is_substruct(from, to);
            bool cast_down = is_substruct(to, from);
            bool valid_cast = cast_up || cast_down;

            CHECK_TYPE(!(cast_up || cast_down), cast->name, "Struct instances must be cast only to superstructs or substructs.");

            emit_byte(compiler, OP_CAST);
            struct ObjString* to_struct = make_string(to->name.start, to->name.length);
            push_root(to_string(to_struct));
            emit_short(compiler, add_constant(compiler, to_string(to_struct)));
            pop_root();

            *node_type =  cast->type;
            return RESULT_SUCCESS;
        }
    } 
}


void init_compiler(struct Compiler* compiler, const char* start, int length, int parameter_count, Token name, struct Type* type) {
    struct ObjString* fun_name = make_string(start, length);
    push_root(to_string(fun_name));
    compiler->scope_depth = 0;
    compiler->locals_count = 0;
    add_local(compiler, name, type); //first slot is for function
    compiler->error_count = 0;
    compiler->function = make_function(fun_name, parameter_count);
    push_root(to_function(compiler->function));
    compiler->enclosing = NULL;
    compiler->upvalue_count = 0;
    compiler->types = NULL;
    compiler->nodes = NULL;
    init_table(&compiler->globals);

    compiler->enclosing = current_compiler;
    current_compiler = compiler;
}

void free_compiler(struct Compiler* compiler) {
    if (compiler->enclosing != NULL) {
        copy_errors(compiler->enclosing, compiler);
    }
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
    //popping function name object and function object pushed onto stack in init_compiler()
    pop_root();
    pop_root();
}

static ResultCode compile_function(struct Compiler* compiler, struct NodeList* nl, struct TypeArray** type_array) {
    struct TypeArray* ret_types = make_type_array();
    for (int i = 0; i < nl->count; i++) {
        struct Type* type;
        ResultCode result = compile_node(compiler, nl->nodes[i], ret_types, &type);
        if (result == RESULT_FAILED) {
            add_error(compiler, make_dummy_token(), "Failed to compile function.");
            print_object((struct Obj*)(compiler->function->name));
            return RESULT_FAILED;
        }
    }

    if ((ret_types)->count == 0) {
        emit_byte(compiler, OP_NIL);
        emit_byte(compiler, OP_RETURN);
    }

    *type_array = ret_types;

    return RESULT_SUCCESS;
}

/*
static Value open_native(int arg_count, Value* args) {
    struct ObjString* file_name = args[0].as.string_type;
    //open file in c (just do read for now)
    //create ObjFile and return
}*/

static Value clock_native(int arg_count, Value* args) {
    return to_float((double)clock() / CLOCKS_PER_SEC);
}

static Value print_native(int arg_count, Value* args) {
    Value value = args[0];
    switch(value.type) {
        case VAL_STRING: {
            struct ObjString* str = value.as.string_type;
            printf("%.*s\n", str->length, str->chars);
            break;
        }
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

static Value input_native(int arg_count, Value* args) {
    char input[100];
    fgets(input, 100, stdin);
    return to_string(make_string(input, strlen(input) - 1));
}

static ResultCode define_native(struct Compiler* compiler, const char* name, Value (*function)(int, Value*), struct Type* type) {
    //set globals in compiler for checks
    struct ObjString* native_string = make_string(name, strlen(name));
    push_root(to_string(native_string));
    Value v;
    if (get_from_table(&script_compiler->globals, native_string, &v)) {
        pop_root();
        add_error(compiler, make_dummy_token(), "The identifier for this function is already used.");
        return RESULT_FAILED;
    }
    set_table(&script_compiler->globals, native_string, to_type(type));
    pop_root();

    emit_byte(compiler, OP_NATIVE);
    Value native = to_native(make_native(native_string, function));
    push_root(native);
    emit_short(compiler, add_constant(compiler, native));
    pop_root();
    emit_byte(compiler, OP_ADD_GLOBAL);

    return RESULT_SUCCESS;
}
/*
static ResultCode define_open(struct Compiler* compiler) {
    struct Type* str_type = make_string_type();
    struct Type* params = make_type_array();
    add_type((struct TypeArray*)params, str_type);
    return define_native(compiler, "open", open_native, make_fun_type(params, make_file_type()));
}*/

static ResultCode define_input(struct Compiler* compiler) {
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_string_type());
    return define_native(compiler, "input", input_native, make_fun_type(make_type_array(), returns));
}

static ResultCode define_clock(struct Compiler* compiler) {
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_float_type());
    return define_native(compiler, "clock", clock_native, make_fun_type(make_type_array(), returns));
}

static ResultCode define_print(struct Compiler* compiler) {
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
    struct TypeArray* sl = make_type_array();
    add_type(sl, str_type);
    struct TypeArray* returns = make_type_array();
    add_type(returns, make_nil_type());
    return define_native(compiler, "print", print_native, make_fun_type(sl, returns));
}


ResultCode compile_script(struct Compiler* compiler, struct NodeList* nl) {
    //TODO:script_compiler is currently used to get access to top compiler globals table
    //  this is kind of messy
    script_compiler = compiler;
    define_clock(compiler);
    define_print(compiler);
    define_input(compiler);

    struct TypeArray* ret_types;
    ResultCode result = compile_function(compiler, nl, &ret_types);

    if (result == RESULT_FAILED || compiler->error_count > 0) {
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

