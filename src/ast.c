#include "ast.h"
#include "memory.h"


/*
 * Declarations
 */

struct Node* make_decl_var(Token name, struct Sig* sig, struct Node* right) {
    DeclVar* decl_var = ALLOCATE_NODE(DeclVar);
    decl_var->name = name;
    decl_var->sig = sig;
    decl_var->right = right;
    decl_var->base.type = NODE_DECL_VAR;

    return (struct Node*)decl_var;
}

struct Node* make_decl_fun(Token name, NodeList parameters, struct Sig* sig, struct Node* body) {
    DeclFun* df = ALLOCATE_NODE(DeclFun);
    df->name = name;
    df->parameters = parameters;
    df->sig = sig;
    df->body = body;
    df->base.type = NODE_DECL_FUN;

    return (struct Node*)df;
}

struct Node* make_decl_class(Token name, NodeList decls, struct Sig* sig) {
    DeclClass* dc = ALLOCATE_NODE(DeclClass);
    dc->name = name;
    dc->decls = decls;
    dc->sig = sig;
    dc->base.type = NODE_DECL_CLASS;

    return (struct Node*)dc;
}

/*
 * Statements
 */

struct Node* make_expr_stmt(struct Node* expr) {
    ExprStmt* es = ALLOCATE_NODE(ExprStmt);
    es->expr = expr;
    es->base.type = NODE_EXPR_STMT;

    return (struct Node*)es;
}

struct Node* make_print(Token name, struct Node* right) {
    Print* print = ALLOCATE_NODE(Print);
    print->name = name;
    print->right = right;
    print->base.type = NODE_PRINT;

    return (struct Node*)print;
}

struct Node* make_block(Token name, NodeList dl) {
    Block* block = ALLOCATE_NODE(Block);
    block->name = name;
    block->decl_list = dl;
    block->base.type = NODE_BLOCK;

    return (struct Node*)block;
}

struct Node* make_if_else(Token name, struct Node* condition, 
                          struct Node* then_block, struct Node* else_block) {
    IfElse* ie = ALLOCATE_NODE(IfElse);
    ie->name = name;
    ie->condition = condition;
    ie->then_block = then_block;
    ie->else_block = else_block; 
    ie->base.type = NODE_IF_ELSE;

    return (struct Node*)ie;
}

struct Node* make_while(Token name, struct Node* condition, 
                        struct Node* then_block) {
    While* wh = ALLOCATE_NODE(While);
    wh->name = name;
    wh->condition = condition;
    wh->then_block = then_block;
    wh->base.type = NODE_WHILE;

    return (struct Node*)wh;
}

struct Node* make_for(Token name, struct Node* initializer, struct Node* condition, 
                      struct Node* update, struct Node* then_block) {
    For* fo = ALLOCATE_NODE(For);
    fo->name = name;
    fo->initializer = initializer;
    fo->condition = condition;
    fo->update = update;
    fo->then_block = then_block;
    fo->base.type = NODE_FOR;

    return (struct Node*)fo;
}


struct Node* make_return(Token name, struct Node* right) {
    Return* ret = ALLOCATE_NODE(Return);
    ret->name = name;
    ret->right = right;
    ret->base.type = NODE_RETURN;

    return (struct Node*)ret;
}

/*
 * Expressions
 */

struct Node* make_literal(Token name) {
    Literal* literal = ALLOCATE_NODE(Literal);
    literal->name = name;
    literal->base.type = NODE_LITERAL;

    return (struct Node*)literal;
}

struct Node* make_unary(Token name, struct Node* right) {
    Unary* unary = ALLOCATE_NODE(Unary);
    unary->name = name;
    unary->right = right;
    unary->base.type = NODE_UNARY;

    return (struct Node*)unary;
}

struct Node* make_binary(Token name, struct Node* left, struct Node* right) {
    Binary* binary = ALLOCATE_NODE(Binary);
    binary->name = name;
    binary->left = left;
    binary->right = right;
    binary->base.type = NODE_BINARY;

    return (struct Node*)binary;
}

struct Node* make_logical(Token name, struct Node* left, struct Node* right) {
    Logical* logical = ALLOCATE_NODE(Logical);
    logical->name = name;
    logical->left = left;
    logical->right = right;
    logical->base.type = NODE_LOGICAL;

    return (struct Node*)logical;
}

struct Node* make_get_var(Token name) {
    GetVar* get_var = ALLOCATE_NODE(GetVar);
    get_var->name = name;
    get_var->base.type = NODE_GET_VAR;

    return (struct Node*)get_var;
}

struct Node* make_set_var(Token name, struct Node* right, bool decl) {
    SetVar* set_var = ALLOCATE_NODE(SetVar);
    set_var->name = name;
    set_var->right = right;
    set_var->decl = decl;
    set_var->base.type = NODE_SET_VAR;

    return (struct Node*)set_var;
}

struct Node* make_call(Token name, NodeList arguments) {
    Call* call = ALLOCATE_NODE(Call);
    call->name = name;
    call->arguments = arguments;
    call->base.type = NODE_CALL;

    return (struct Node*)call;
}


struct Node* make_cascade_call(Token name, struct Node* function, NodeList arguments) {
    CascadeCall* call = ALLOCATE_NODE(CascadeCall);
    call->name = name;
    call->function = function;
    call->arguments = arguments;
    call->base.type = NODE_CASCADE_CALL;

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
        case NODE_GET_VAR: {
            //TODO: fill this in
            printf("GetVar");
            break;
        }
        case NODE_SET_VAR: {
            //TODO: fill this in
            printf("SetVar");
            break;
        }
        case NODE_CALL: {
            //TODO: fill this in
            printf("Call");
            break;
        }
        case NODE_CASCADE_CALL: {
            //TODO: fill this in
            printf("Call");
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
            free_sig(decl_var->sig);
            FREE_NODE(decl_var, DeclVar);
            break;
        }
        case NODE_DECL_FUN: {
            DeclFun* df = (DeclFun*)node;
            free_node_list(&df->parameters);
            free_node(df->body);
            free_sig(df->sig);
            FREE_NODE(df, DeclFun);
            break;
        }
        case NODE_DECL_CLASS: {
            DeclClass* dc = (DeclClass*)node;
            free_node_list(&dc->decls);
            FREE_NODE(dc, DeclClass);
            break;
        }
        //Statements
        case NODE_EXPR_STMT: {
            ExprStmt* es = (ExprStmt*)node;
            free_node(es->expr);
            FREE_NODE(es, ExprStmt);
            break;
        }
        case NODE_PRINT: {
            Print* print = (Print*)node;
            free_node(print->right);
            FREE_NODE(print, Print);
            break;
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            free_node_list(&block->decl_list);
            FREE_NODE(block, Block);
            break;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            free_node(ie->condition);
            free_node(ie->then_block);
            if (ie->else_block != NULL) {
                free_node(ie->else_block);
            }
            FREE_NODE(ie, IfElse);
            break;
        }
        case NODE_WHILE: {
            While* wh = (While*)node;
            free_node(wh->condition);
            free_node(wh->then_block);
            FREE_NODE(wh, While);
            break;
        }
        case NODE_FOR: {
            For* fo = (For*)node;
            if (fo->initializer != NULL) free_node(fo->initializer);
            free_node(fo->condition);
            if (fo->update != NULL) free_node(fo->update);
            free_node(fo->then_block);
            FREE_NODE(fo, For);
            break;
        }
        case NODE_RETURN: {
            Return* ret = (Return*)node;
            if (ret->right != NULL) free_node(ret->right);
            FREE_NODE(ret, Return);
            break;
        }
        //struct Expressions
        case NODE_LITERAL: {
            Literal* literal = (Literal*)node;
            FREE_NODE(literal, Literal);
            break;
        }
        case NODE_BINARY: {
            Binary* binary = (Binary*)node;
            free_node(binary->left);
            free_node(binary->right);
            FREE_NODE(binary, Binary);
            break;
        }
        case NODE_LOGICAL: {
            Logical* binary = (Logical*)node;
            free_node(binary->left);
            free_node(binary->right);
            FREE_NODE(binary, Logical);
            break;
        }
        case NODE_UNARY: {
            Unary* unary = (Unary*)node;
            free_node(unary->right);
            FREE_NODE(unary, Unary);
            break;
        }
        case NODE_GET_VAR: {
            GetVar* gv = (GetVar*)node;
            FREE_NODE(gv, GetVar);
            break;
        }
        case NODE_SET_VAR: {
            SetVar* sv = (SetVar*)node;
            free_node(sv->right);
            FREE_NODE(sv, SetVar);
            break;
        }
        case NODE_CALL: {
            Call* call = (Call*)node;
            free_node_list(&call->arguments);
            FREE_NODE(call, Call);
            break;
        }
        case NODE_CASCADE_CALL: {
            CascadeCall* call = (CascadeCall*)node;
            free_node(call->function);
            free_node_list(&call->arguments);
            FREE_NODE(call, CascadeCall);
            break;
        }
    } 
}

