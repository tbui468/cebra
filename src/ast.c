#include "ast.h"
#include "memory.h"

//also init 'has_declarations'
struct Node* insert_node(struct Node* node) {
    node->has_decls = false; //what is this?
    node->next = current_compiler->nodes;
    current_compiler->nodes = node;    

    return node;
}

struct Node* make_node_list() {
    struct NodeList* nl = ALLOCATE(struct NodeList);
    nl->nodes = ALLOCATE_ARRAY(struct Node*);
    nl->count = 0;
    nl->capacity = 0;
    nl->base.type = NODE_LIST;

    return insert_node((struct Node*)nl);
}

struct Node* make_sequence(Token op, struct NodeList* left, struct Node* right) {
    struct Sequence* seq = ALLOCATE(struct Sequence);
    seq->op = op;
    seq->left = left;
    seq->right = right;
    seq->base.type = NODE_SEQUENCE;

    return insert_node((struct Node*)seq);
}

void add_node(struct NodeList* nl, struct Node* node) {
    if (nl->count + 1 > nl->capacity) {
        int new_capacity = nl->capacity == 0 ? 8 : nl->capacity * 2;
        nl->nodes = GROW_ARRAY(nl->nodes, struct Node*, new_capacity, nl->capacity);
        nl->capacity = new_capacity;
    }

    nl->nodes[nl->count] = node;
    nl->count++;
}

/*
 * Declarations
 */

struct Node* make_decl_var(Token name, struct Type* type, struct Node* right) {
    DeclVar* decl_var = ALLOCATE(DeclVar);
    decl_var->name = name;
    decl_var->type = type;
    decl_var->right = right;
    decl_var->base.type = NODE_DECL_VAR;

    return insert_node((struct Node*)decl_var);
}

struct Node* make_decl_fun(Token name, struct NodeList* parameters, struct Type* type, struct Node* body, bool anonymous) {
    DeclFun* df = ALLOCATE(DeclFun);
    df->name = name;
    df->parameters = parameters;
    df->type = type;
    df->body = body;
    df->anonymous = anonymous;
    df->base.type = NODE_FUN;

    return insert_node((struct Node*)df);
}

struct Node* make_decl_struct(Token name, struct Node* super, struct NodeList* decls) {
    struct DeclStruct* dc = ALLOCATE(struct DeclStruct);
    dc->name = name;
    dc->super = super;
    dc->decls = decls;
    dc->base.type = NODE_STRUCT;

    return insert_node((struct Node*)dc);
}

struct Node* make_decl_enum(Token name, struct NodeList* decls) {
    struct DeclEnum* de = ALLOCATE(struct DeclEnum);
    de->name = name;
    de->identifier = make_literal(name);
    de->decls = decls;
    de->base.type = NODE_ENUM;

    return insert_node((struct Node*)de);
}

struct Node* make_decl_container(Token name, struct Type* type) {
    struct DeclContainer* dc = ALLOCATE(struct DeclContainer);
    dc->name = name;
    dc->type = type;
    dc->base.type = NODE_CONTAINER;

    return insert_node((struct Node*)dc);
}

/*
 * Statements
 */

struct Node* make_expr_stmt(struct Node* expr) {
    ExprStmt* es = ALLOCATE(ExprStmt);
    es->expr = expr;
    es->base.type = NODE_EXPR_STMT;

    return insert_node((struct Node*)es);
}

struct Node* make_block(Token name, struct NodeList* dl) {
    Block* block = ALLOCATE(Block);
    block->name = name;
    block->decl_list = dl;
    block->base.type = NODE_BLOCK;

    return insert_node((struct Node*)block);
}

struct Node* make_if_else(Token name, struct Node* condition, 
                          struct Node* then_block, struct Node* else_block) {
    IfElse* ie = ALLOCATE(IfElse);
    ie->name = name;
    ie->condition = condition;
    ie->then_block = then_block;
    ie->else_block = else_block; 
    ie->base.type = NODE_IF_ELSE;

    return insert_node((struct Node*)ie);
}

struct Node* make_when(Token name, struct NodeList* cases) {
    struct When* when = ALLOCATE(struct When);
    when->name = name;
    when->cases = cases;
    when->base.type = NODE_WHEN;

    return insert_node((struct Node*)when);
}

struct Node* make_while(Token name, struct Node* condition, 
                        struct Node* then_block) {
    While* wh = ALLOCATE(While);
    wh->name = name;
    wh->condition = condition;
    wh->then_block = then_block;
    wh->base.type = NODE_WHILE;

    return insert_node((struct Node*)wh);
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

    return insert_node((struct Node*)fo);
}

struct Node* make_return(Token name, struct Node* right) {
    Return* ret = ALLOCATE(Return);
    ret->name = name;
    ret->right = right;
    ret->base.type = NODE_RETURN;

    return insert_node((struct Node*)ret);
}

/*
 * Expressions
 */

struct Node* make_literal(Token name) {
    Literal* literal = ALLOCATE(Literal);
    literal->name = name;
    literal->base.type = NODE_LITERAL;

    return insert_node((struct Node*)literal);
}

struct Node* make_unary(Token name, struct Node* right) {
    Unary* unary = ALLOCATE(Unary);
    unary->name = name;
    unary->right = right;
    unary->base.type = NODE_UNARY;

    return insert_node((struct Node*)unary);
}

struct Node* make_binary(Token name, struct Node* left, struct Node* right) {
    Binary* binary = ALLOCATE(Binary);
    binary->name = name;
    binary->left = left;
    binary->right = right;
    binary->base.type = NODE_BINARY;

    return insert_node((struct Node*)binary);
}

struct Node* make_logical(Token name, struct Node* left, struct Node* right) {
    Logical* logical = ALLOCATE(Logical);
    logical->name = name;
    logical->left = left;
    logical->right = right;
    logical->base.type = NODE_LOGICAL;

    return insert_node((struct Node*)logical);
}

struct Node* make_get_prop(struct Node* inst, Token prop) {
    GetProp* get_prop = ALLOCATE(GetProp);
    get_prop->inst = inst;
    get_prop->prop = prop;
    get_prop->base.type = NODE_GET_PROP;

    return insert_node((struct Node*)get_prop);
}

struct Node* make_set_prop(struct Node* inst, struct Node* right) {
    SetProp* set_prop = ALLOCATE(SetProp);
    set_prop->inst = inst;
    set_prop->right = right;
    set_prop->base.type = NODE_SET_PROP;

    return insert_node((struct Node*)set_prop);
}

struct Node* make_get_var(Token name) {
    GetVar* get_var = ALLOCATE(GetVar);
    get_var->name = name;
    get_var->base.type = NODE_GET_VAR;

    return insert_node((struct Node*)get_var);
}

struct Node* make_set_var(struct Node* left, struct Node* right) {
    SetVar* set_var = ALLOCATE(SetVar);
    set_var->left = left;
    set_var->right = right;
    set_var->base.type = NODE_SET_VAR;

    return insert_node((struct Node*)set_var);
}


struct Node* make_slice_string(Token name, struct Node* left, struct Node* start_idx, struct Node* end_idx) {
    SliceString* ss = ALLOCATE(SliceString);
    ss->name = name;
    ss->left = left;
    ss->start_idx = start_idx;
    ss->end_idx = end_idx;
    ss->base.type = NODE_SLICE_STRING;

    return insert_node((struct Node*)ss);
}

struct Node* make_get_element(Token name, struct Node* left, struct Node* idx) {
    GetElement* get_ele = ALLOCATE(GetElement);
    get_ele->name = name;
    get_ele->left = left;
    get_ele->idx = idx;
    get_ele->base.type = NODE_GET_ELEMENT;

    return insert_node((struct Node*)get_ele);
}

struct Node* make_set_element(struct Node* left, struct Node* right) {
    SetElement* set_ele = ALLOCATE(SetElement);
    set_ele->left = left;
    set_ele->right = right;
    set_ele->base.type = NODE_SET_ELEMENT;

    return insert_node((struct Node*)set_ele);
}

struct Node* make_call(Token name, struct Node* left, struct NodeList* arguments) {
    Call* call = ALLOCATE(Call);
    call->name = name;
    call->left = left;
    call->arguments = arguments;
    call->base.type = NODE_CALL;

    return insert_node((struct Node*)call);
}

struct Node* make_nil(Token name) {
    Nil* nil = ALLOCATE(Nil);
    nil->name = name;
    nil->base.type = NODE_NIL;

    return insert_node((struct Node*)nil);
}

struct Node* make_cast(Token name, struct Node* left, struct Type* type) {
    Cast* cast = ALLOCATE(Cast);
    cast->name = name;
    cast->left = left;
    cast->type = type;
    cast->base.type = NODE_CAST;

    return insert_node((struct Node*)cast);
}

/*
 * Utility
 */

void print_node(struct Node* node) {
    switch(node->type) {
        case NODE_LIST: {
            struct NodeList* nl = (struct NodeList*)node;
            printf("( NodeList \n");
            for (int i = 0; i < nl->count; i++) {
                print_node(nl->nodes[i]);
                printf("\n");
            }
            printf(")");
            break;
        }
        case NODE_SEQUENCE: {
            struct Sequence* seq = (struct Sequence*)node;
            printf("( NodeSequence ");
            printf("left count: %d", seq->left->count);
            printf(")");
            break;
        }
        //Declarations
        case NODE_DECL_VAR: {
            DeclVar* dv = (DeclVar*)node;
            printf("( DeclVar %.*s ", dv->name.length, dv->name.start);
            if (dv->right != NULL) print_node(dv->right);
            printf(")");
            break;
        }
        case NODE_FUN: {
            DeclFun* df = (DeclFun*)node;
            printf("( DeclFun ");
            print_token(df->name);
     //       print_node(df->body);
            printf(" )");
            break;
        }
        case NODE_STRUCT: {
            struct DeclStruct* dc = (struct DeclStruct*)node;
            printf("( DeclStruct ");
            print_token(dc->name);
            printf(" )");
            break;
        }
        case NODE_ENUM: {
            struct DeclEnum* de = (struct DeclEnum*)node;
            printf("( DeclEnum ");
            break;
        }
        case NODE_CONTAINER: {
            struct DeclContainer* de = (struct DeclContainer*)node;
            printf("( DeclContainer ");
            break;
        }
        //Statements
        case NODE_EXPR_STMT: {
            ExprStmt* es = (ExprStmt*)node;
            printf("Expr Stmt");
            break;
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            printf("( Block");
            print_node((struct Node*)(block->decl_list));
            printf(" )");
            break;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            printf("( If ");
            /*
            print_node(ie->condition);
            printf(" then ");
            print_node(ie->then_block);
            if (ie->else_block != NULL) {
                printf(" else ");
                print_node(ie->else_block);
            }*/
            printf(" )");
            break;
        }
        case NODE_WHEN: {
            printf("( When )");
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
            printf("( Literal [%.*s] )", literal->name.length, literal->name.start);
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
            GetVar* gv = (GetVar*)node;
            printf("( GetVar ");
            print_token(gv->name);
            printf(" )");
            break;
        }
        case NODE_SET_VAR: {
            printf("SetVar stub");
            break;
        }
        case NODE_SLICE_STRING: {
            printf("Slice String");
            break;
        }
        case NODE_GET_ELEMENT: {
            GetElement* gi = (GetElement*)node;
            printf("( GetElement ");
            if (gi->left != NULL) print_node(gi->left);
            if (gi->idx != NULL) print_node(gi->idx);
            printf(" )");
            break;
        }
        case NODE_SET_ELEMENT: {
            printf("SetElement stub");
            break;
        }
        case NODE_CALL: {
            printf("Call stub");
            break;
        }
        case NODE_NIL: {
            printf("Nil stub");
            break;
        }
        case NODE_CAST: {
            printf("Cast stub");
            break;
        }
    } 
}


void free_node(struct Node* node) {
    if (node == NULL) return;
    switch(node->type) {
        case NODE_LIST: {
            struct NodeList* nl = (struct NodeList*)node;
            FREE_ARRAY(nl->nodes, struct Node*, nl->capacity);
            FREE(nl, struct NodeList);
            break;
        }
        case NODE_SEQUENCE: {
            struct Sequence* seq = (struct Sequence*)node;
            FREE(seq, struct Sequence);
            break;
        }
        //Declarations
        case NODE_DECL_VAR: {
            DeclVar* decl_var = (DeclVar*)node;
            FREE(decl_var, DeclVar);
            break;
        }
        case NODE_FUN: {
            DeclFun* df = (DeclFun*)node;
            FREE(df, DeclFun);
            break;
        }
        case NODE_STRUCT: {
            struct DeclStruct* dc = (struct DeclStruct*)node;
            FREE(dc, struct DeclStruct);
            break;
        }
        case NODE_ENUM: {
            struct DeclEnum* de = (struct DeclEnum*)node;
            FREE(de, struct DeclEnum);
            break;
        }
        case NODE_CONTAINER: {
            struct DeclContainer* de = (struct DeclContainer*)node;
            FREE(de, struct DeclContainer);
            break;
        }
        //Statements
        case NODE_EXPR_STMT: {
            ExprStmt* es = (ExprStmt*)node;
            FREE(es, ExprStmt);
            break;
        }
        case NODE_BLOCK: {
            Block* block = (Block*)node;
            FREE(block, Block);
            break;
        }
        case NODE_IF_ELSE: {
            IfElse* ie = (IfElse*)node;
            FREE(ie, IfElse);
            break;
        }
        case NODE_WHEN: {
            struct When* when = (struct When*)node;
            FREE(when, struct When);
            break;
        }
        case NODE_WHILE: {
            While* wh = (While*)node;
            FREE(wh, While);
            break;
        }
        case NODE_FOR: {
            For* fo = (For*)node;
            FREE(fo, For);
            break;
        }
        case NODE_RETURN: {
            Return* ret = (Return*)node;
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
            FREE(binary, Binary);
            break;
        }
        case NODE_LOGICAL: {
            Logical* binary = (Logical*)node;
            FREE(binary, Logical);
            break;
        }
        case NODE_UNARY: {
            Unary* unary = (Unary*)node;
            FREE(unary, Unary);
            break;
        }
        case NODE_GET_PROP: {
            GetProp* get_prop = (GetProp*)node;
            FREE(get_prop, GetProp);
            break;
        }
        case NODE_SET_PROP: {
            SetProp* set_prop = (SetProp*)node;
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
            FREE(sv, SetVar);
            break;
        }
        case NODE_SLICE_STRING: {
            SliceString* ss = (SliceString*)node;
            FREE(ss, SliceString);
            break;
        }
        case NODE_GET_ELEMENT: {
            GetElement* gv = (GetElement*)node;
            FREE(gv, GetElement);
            break;
        }
        case NODE_SET_ELEMENT: {
            SetElement* sv = (SetElement*)node;
            FREE(sv, SetElement);
            break;
        }
        case NODE_CALL: {
            Call* call = (Call*)node;
            FREE(call, Call);
            break;
        }
        case NODE_NIL: {
            Nil* nil = (Nil*)node;
            FREE(nil, Nil);
            break;
        }
        case NODE_CAST: {
            Cast* cast = (Cast*)node;
            FREE(cast, Cast);
            break;
        }
    } 
}
