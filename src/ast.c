#include "ast.h"
#include "memory.h"


/*
 * Declarations
 */

struct Node* make_decl_var(Token name, struct Sig* sig, struct Node* right) {
    DeclVar* decl_var = ALLOCATE(DeclVar);
    decl_var->name = name;
    decl_var->sig = sig;
    decl_var->right = right;
    decl_var->base.type = NODE_DECL_VAR;

    return (struct Node*)decl_var;
}

struct Node* make_decl_fun(Token name, NodeList parameters, struct Sig* sig, struct Node* body) {
    DeclFun* df = ALLOCATE(DeclFun);
    df->name = name;
    df->parameters = parameters;
    df->sig = sig;
    df->body = body;
    df->base.type = NODE_DECL_FUN;

    return (struct Node*)df;
}

struct Node* make_decl_class(Token name, NodeList decls, struct Sig* sig) {
    DeclClass* dc = ALLOCATE(DeclClass);
    dc->name = name;
    dc->decls = decls;
    dc->sig = sig;
    dc->base.type = NODE_DECL_CLASS;

    return (struct Node*)dc;
}

struct Node* make_inst_class(Token name, Token klass_type, struct Node* klass) {
    InstClass* ic = ALLOCATE(InstClass);
    ic->name = name;
    ic->klass_type = klass_type;
    ic->klass = klass;
    ic->sig = NULL; //filled in by compiler since class definition needed
    ic->base.type = NODE_INST_CLASS;

    return (struct Node*)ic;
}

/*
 * Statements
 */

struct Node* make_expr_stmt(struct Node* expr) {
    ExprStmt* es = ALLOCATE(ExprStmt);
    es->expr = expr;
    es->base.type = NODE_EXPR_STMT;

    return (struct Node*)es;
}

struct Node* make_print(Token name, struct Node* right) {
    Print* print = ALLOCATE(Print);
    print->name = name;
    print->right = right;
    print->base.type = NODE_PRINT;

    return (struct Node*)print;
}

struct Node* make_block(Token name, NodeList dl) {
    Block* block = ALLOCATE(Block);
    block->name = name;
    block->decl_list = dl;
    block->base.type = NODE_BLOCK;

    return (struct Node*)block;
}

struct Node* make_if_else(Token name, struct Node* condition, 
                          struct Node* then_block, struct Node* else_block) {
    IfElse* ie = ALLOCATE(IfElse);
    ie->name = name;
    ie->condition = condition;
    ie->then_block = then_block;
    ie->else_block = else_block; 
    ie->base.type = NODE_IF_ELSE;

    return (struct Node*)ie;
}

struct Node* make_while(Token name, struct Node* condition, 
                        struct Node* then_block) {
    While* wh = ALLOCATE(While);
    wh->name = name;
    wh->condition = condition;
    wh->then_block = then_block;
    wh->base.type = NODE_WHILE;

    return (struct Node*)wh;
}

struct Node* make_for(Token name, struct Node* initializer, struct Node* condition, 
                      struct Node* update, struct Node* then_block) {
    For* fo = ALLOCATE(For);
    fo->name = name;
    fo->initializer = initializer;
    fo->condition = condition;
    fo->update = update;
    fo->then_block = then_block;
    fo->base.type = NODE_FOR;

    return (struct Node*)fo;
}


struct Node* make_return(Token name, struct Node* right) {
    Return* ret = ALLOCATE(Return);
    ret->name = name;
    ret->right = right;
    ret->base.type = NODE_RETURN;

    return (struct Node*)ret;
}

/*
 * Expressions
 */

struct Node* make_literal(Token name) {
    Literal* literal = ALLOCATE(Literal);
    literal->name = name;
    literal->base.type = NODE_LITERAL;

    return (struct Node*)literal;
}

struct Node* make_unary(Token name, struct Node* right) {
    Unary* unary = ALLOCATE(Unary);
    unary->name = name;
    unary->right = right;
    unary->base.type = NODE_UNARY;

    return (struct Node*)unary;
}

struct Node* make_binary(Token name, struct Node* left, struct Node* right) {
    Binary* binary = ALLOCATE(Binary);
    binary->name = name;
    binary->left = left;
    binary->right = right;
    binary->base.type = NODE_BINARY;

    return (struct Node*)binary;
}

struct Node* make_logical(Token name, struct Node* left, struct Node* right) {
    Logical* logical = ALLOCATE(Logical);
    logical->name = name;
    logical->left = left;
    logical->right = right;
    logical->base.type = NODE_LOGICAL;

    return (struct Node*)logical;
}

struct Node* make_get_prop(struct Node* inst, Token prop) {
    GetProp* get_prop = ALLOCATE(GetProp);
    get_prop->inst = inst;
    get_prop->prop = prop;
    get_prop->base.type = NODE_GET_PROP;

    return (struct Node*)get_prop;
}

struct Node* make_set_prop(struct Node* inst, Token prop, struct Node* right) {
    SetProp* set_prop = ALLOCATE(SetProp);
    set_prop->inst = inst;
    set_prop->prop = prop;
    set_prop->right = right;
    set_prop->base.type = NODE_SET_PROP;

    return (struct Node*)set_prop;
}

struct Node* make_get_var(Token name) {
    GetVar* get_var = ALLOCATE(GetVar);
    get_var->name = name;
    get_var->base.type = NODE_GET_VAR;

    return (struct Node*)get_var;
}

struct Node* make_set_var(struct Node* left, struct Node* right) {
    SetVar* set_var = ALLOCATE(SetVar);
    set_var->left = left;
    set_var->right = right;
    set_var->base.type = NODE_SET_VAR;

    return (struct Node*)set_var;
}

struct Node* make_call(Token name, struct Node* left, NodeList arguments) {
    Call* call = ALLOCATE(Call);
    call->name = name;
    call->left = left;
    call->arguments = arguments;
    call->base.type = NODE_CALL;

    return (struct Node*)call;
}


/*
 * Utility
 */

void print_node(struct Node* node) {
    switch(node->type) {
        //Declarations
        case NODE_DECL_VAR: {
            DeclVar* dv = (DeclVar*)node;
            printf("( DeclVar %.*s ", dv->name.length, dv->name.start);
            print_node(dv->right);
            printf(")");
            break;
        }
        case NODE_DECL_FUN: {
            DeclFun* df = (DeclFun*)node;
            printf("( DeclFun ");
            print_node(df->body);
            break;
        }
        case NODE_DECL_CLASS: {
            DeclClass* dc = (DeclClass*)node;
            printf("( DeclClass ");
            break;
        }
        case NODE_INST_CLASS: {
            printf("( InstClass Stub  )");
            break;
        }
        //Statements
        case NODE_EXPR_STMT: {
            ExprStmt* es = (ExprStmt*)node;
            printf("Expr Stmt");
            break;
        }
        case NODE_PRINT: {
            Print* print = (Print*)node;
            printf("( %.*s ", print->name.length, print->name.start);
            print_node(print->right);
            printf(")");
            break;
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            printf("( Block");
            print_node_list(&block->decl_list);
            printf(" )");
            break;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            printf("( If ");
            print_node(ie->condition);
            printf(" then ");
            print_node(ie->then_block);
            if (ie->else_block != NULL) {
                printf(" else ");
                print_node(ie->else_block);
            }
            printf(" )");
            break;
        }
        case NODE_WHILE: {
            //TODO: fill this in
            printf("While");
            break;
        }
        case NODE_FOR: {
            //TODO: fill this in
            printf("For");
            break;
        }
        case NODE_RETURN: {
            //TODO: fill this in
            printf("Return");
            break;
        }
        //struct Expressions
        case NODE_LITERAL: {
            Literal* literal = (Literal*)node;
            printf(" %.*s ", literal->name.length, literal->name.start);
            break;
        }
        case NODE_BINARY: {
            Binary* binary = (Binary*)node;
            printf("( %.*s ", binary->name.length, binary->name.start);
            print_node(binary->left);
            print_node(binary->right);
            printf(")");
            break;
        }
        case NODE_LOGICAL: {
            Logical* binary = (Logical*)node;
            printf("( %.*s ", binary->name.length, binary->name.start);
            print_node(binary->left);
            print_node(binary->right);
            printf(")");
            break;
        }
        case NODE_UNARY: {
            Unary* unary = (Unary*)node;
            printf("( %.*s ", unary->name.length, unary->name.start);
            print_node(unary->right);
            printf(")");
            break;
        }
        case NODE_GET_PROP: {
            printf("GetProp stub");
            break;
        }
        case NODE_SET_PROP: {
            printf("SetProp stub");
            break;
        }
        case NODE_GET_VAR: {
            printf("GetVar stub");
            break;
        }
        case NODE_SET_VAR: {
            printf("SetVar stub");
            break;
        }
        case NODE_CALL: {
            printf("Call stub");
            break;
        }
    } 
}


void free_node(struct Node* node) {
    switch(node->type) {
        //Declarations
        case NODE_DECL_VAR: {
            DeclVar* decl_var = (DeclVar*)node;
            //parameters have NULL for right expression
            if (decl_var->right != NULL) {
                free_node(decl_var->right);
            }
            FREE(decl_var, DeclVar);
            break;
        }
        case NODE_DECL_FUN: {
            DeclFun* df = (DeclFun*)node;
            free_node_list(&df->parameters);
            free_node(df->body);
            FREE(df, DeclFun);
            break;
        }
        case NODE_DECL_CLASS: {
            DeclClass* dc = (DeclClass*)node;
            free_node_list(&dc->decls);
            FREE(dc, DeclClass);
            break;
        }
        case NODE_INST_CLASS: {
            InstClass* ic = (InstClass*)node;
            free_node(ic->klass);
            FREE(ic, InstClass);
            break;
        }
        //Statements
        case NODE_EXPR_STMT: {
            ExprStmt* es = (ExprStmt*)node;
            free_node(es->expr);
            FREE(es, ExprStmt);
            break;
        }
        case NODE_PRINT: {
            Print* print = (Print*)node;
            free_node(print->right);
            FREE(print, Print);
            break;
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            free_node_list(&block->decl_list);
            FREE(block, Block);
            break;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            free_node(ie->condition);
            free_node(ie->then_block);
            if (ie->else_block != NULL) {
                free_node(ie->else_block);
            }
            FREE(ie, IfElse);
            break;
        }
        case NODE_WHILE: {
            While* wh = (While*)node;
            free_node(wh->condition);
            free_node(wh->then_block);
            FREE(wh, While);
            break;
        }
        case NODE_FOR: {
            For* fo = (For*)node;
            if (fo->initializer != NULL) free_node(fo->initializer);
            free_node(fo->condition);
            if (fo->update != NULL) free_node(fo->update);
            free_node(fo->then_block);
            FREE(fo, For);
            break;
        }
        case NODE_RETURN: {
            Return* ret = (Return*)node;
            if (ret->right != NULL) free_node(ret->right);
            FREE(ret, Return);
            break;
        }
        //struct Expressions
        case NODE_LITERAL: {
            Literal* literal = (Literal*)node;
            FREE(literal, Literal);
            break;
        }
        case NODE_BINARY: {
            Binary* binary = (Binary*)node;
            free_node(binary->left);
            free_node(binary->right);
            FREE(binary, Binary);
            break;
        }
        case NODE_LOGICAL: {
            Logical* binary = (Logical*)node;
            free_node(binary->left);
            free_node(binary->right);
            FREE(binary, Logical);
            break;
        }
        case NODE_UNARY: {
            Unary* unary = (Unary*)node;
            free_node(unary->right);
            FREE(unary, Unary);
            break;
        }
        case NODE_GET_PROP: {
            GetProp* get_prop = (GetProp*)node;
            free_node(get_prop->inst);
            FREE(get_prop, GetProp);
            break;
        }
        case NODE_SET_PROP: {
            SetProp* set_prop = (SetProp*)node;
            free_node(set_prop->inst);
            free_node(set_prop->right);
            FREE(set_prop, SetProp);
            break;
        }
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;
            FREE(gv, GetVar);
            break;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            free_node(sv->left);
            free_node(sv->right);
            FREE(sv, SetVar);
            break;
        }
        case NODE_CALL: {
            Call* call = (Call*)node;
            free_node(call->left);
            free_node_list(&call->arguments);
            FREE(call, Call);
            break;
        }
    } 
}
