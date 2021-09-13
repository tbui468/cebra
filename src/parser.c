#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "memory.h"

Parser parser;

static struct Node* expression();
static struct Node* declaration();
static void add_error(Token token, const char* message);

static void print_all_tokens() {
    printf("******start**********\n");
    printf("%d\n", parser.previous.type);
    printf("%d\n", parser.current.type);
    printf("%d\n", parser.next.type);
    printf("%d\n", parser.next_next.type);
    printf("******end**********\n");
}

static bool match(TokenType type) {
    if (parser.current.type == type) {
        parser.previous = parser.current;
        parser.current = parser.next;
        parser.next = parser.next_next;
        parser.next_next = next_token();
        return true;
    }

    return false;
}

static void consume(TokenType type, const char* message) {
    if (type == parser.current.type) {
        parser.previous = parser.current;
        parser.current = parser.next;
        parser.next = parser.next_next;
        parser.next_next = next_token();
        return;
    }
    
    add_error(parser.previous, message);
}

static bool peek_one(TokenType type) {
    return parser.current.type == type;
}

static bool peek_two(TokenType type1, TokenType type2) {
    return parser.current.type == type1 && parser.next.type == type2;
}
static bool peek_three(TokenType type1, TokenType type2, TokenType type3) {
    return parser.current.type == type1 && 
           parser.next.type == type2 &&
           parser.next_next.type == type3;
}


static NodeList argument_list() {
//    match(TOKEN_LEFT_PAREN);
    NodeList args;
    init_node_list(&args);
    if (!match(TOKEN_RIGHT_PAREN)) {
        do {
            add_node(&args, expression()); 
        } while (match(TOKEN_COMMA));
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    }

    return args;
}

static struct Node* call_expression() {
    match(TOKEN_IDENTIFIER);
    struct Node* left = make_get_var(parser.previous);
    match(TOKEN_LEFT_PAREN);
    Token name = parser.previous;
    return make_call(name, left, argument_list());
}

/*
static struct Node* cascade_expression() {
    return make_call(make_dummy_token(), argument_list());
}*/

static struct Node* primary() {
    if (match(TOKEN_INT) || match(TOKEN_FLOAT) || 
        match(TOKEN_STRING) || match(TOKEN_TRUE) ||
        match(TOKEN_FALSE)) {
        return make_literal(parser.previous);
        /*
    } else if (peek_three(TOKEN_IDENTIFIER, TOKEN_DOT, TOKEN_IDENTIFIER)) {
        match(TOKEN_IDENTIFIER);
        Token inst_name = parser.previous;
        match(TOKEN_DOT);
        match(TOKEN_IDENTIFIER);
        Token prop_name = parser.previous;

        if (match(TOKEN_EQUAL)) {
            return make_set_prop(inst_name, prop_name, expression());
        }
    
        return make_get_prop(inst_name, prop_name);
    } else if (peek_two(TOKEN_IDENTIFIER, TOKEN_LEFT_PAREN)) {
        return call_expression();
    } else if (peek_one(TOKEN_LEFT_PAREN)) {
        return cascade_expression();*/
    } else if (match(TOKEN_IDENTIFIER)) {
        Token name = parser.previous;
        return make_get_var(name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        struct Node* expr = expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    }

    return NULL;
}

static struct Node* call_dot() {
    struct Node* left = primary();

    while (!peek_three(TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_EQUAL) && 
            (match(TOKEN_DOT) || match(TOKEN_LEFT_PAREN))) {
        if (parser.previous.type == TOKEN_DOT) {
            consume(TOKEN_IDENTIFIER, "Expect identifier after '.'.");
            //NOTE: Need to skip this and return to assignment() if TOKEN_DOT is followed by TOKEN_IDENTIFER and TOKEN_EQUAL
            //  since it's SetProp, and we need the inst, prop and rhs expression for SetProp
            //if (left->type != NODE_GET_VAR) add_error(((GetVar*)left)->name, "Left side must be instance name");
            left = make_get_prop(left, parser.previous);
        } else { //TOKEN_PAREN
            //if (left->type != NODE_GET_VAR) add_error(((GetVar*)left)->name, "Left side must be function name");
            left = make_call(parser.previous, left, argument_list());
        }
    }

    return left;
}

static struct Node* unary() {
    if (match(TOKEN_MINUS) || match(TOKEN_BANG)) {
        return make_unary(parser.previous, unary());
    }

    return call_dot();
}

static struct Node* factor() {
    struct Node* left = unary();

    while (match(TOKEN_STAR) || match(TOKEN_SLASH) || match(TOKEN_MOD)) {
        Token name = parser.previous;
        struct Node* right = unary();
        left = make_binary(name, left, right);
    }

    return left;
}

static struct Node* term() {
    struct Node* left = factor();

    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        Token name = parser.previous;
        struct Node* right = factor();
        left = make_binary(name, left, right);
    }

    return left;
}

static struct Node* relation() {
    struct Node* left = term();

    while (match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL) ||
           match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL)) {
        Token name = parser.previous;
        struct Node* right = term();
        left = make_logical(name, left, right);
    }

    return left;
}

static struct Node* equality() {
    struct Node* left = relation();

    while (match(TOKEN_EQUAL_EQUAL) || match(TOKEN_BANG_EQUAL)) {
        Token name = parser.previous;
        struct Node* right = relation();
        left = make_logical(name, left, right);
    }

    return left;
}

static struct Node* and() {
    struct Node* left = equality();

    while (match(TOKEN_AND)) {
        Token name = parser.previous;
        struct Node* right = equality();
        left = make_logical(name, left, right);
    }

    return left;
}

static struct Node* or() {
    struct Node* left = and();

    while (match(TOKEN_OR)) {
        Token name = parser.previous;
        struct Node* right = and();
        left = make_logical(name, left, right);
    }

    return left;
}

static struct Node* assignment() {
    struct Node* left = or();

    while (match(TOKEN_EQUAL) || peek_three(TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_EQUAL)) {
        if (left->type == NODE_GET_VAR) {
            left = make_set_var(left, expression());
        } else {
            left = make_set_prop(left, parser.previous, expression());
        }
    }

    return left;
}

static struct Node* expression() {
    return assignment();
}

static struct Node* block() {
    Token name = parser.previous;
    NodeList body;
    init_node_list(&body);
    while (!match(TOKEN_RIGHT_BRACE)) {
        add_node(&body, declaration());
    }
    return make_block(name, body);
}

static struct Sig* read_sig(Token name) {
    if (match(TOKEN_LEFT_PAREN)) {
        struct Sig* params = make_list_sig();
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                add_sig((struct SigList*)params, read_sig(name));
            } while(match(TOKEN_COMMA));
            consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameter types.");
        }
        consume(TOKEN_RIGHT_ARROW, "Expect '->' followed by return type.");
        struct Sig* ret = read_sig(name);
        return make_fun_sig(params, ret);
    }

    if (match(TOKEN_INT_TYPE)) 
        return make_prim_sig(VAL_INT);

    if (match(TOKEN_FLOAT_TYPE))
        return make_prim_sig(VAL_FLOAT);
    
    if (match(TOKEN_BOOL_TYPE))
        return make_prim_sig(VAL_BOOL);
    
    if (match(TOKEN_STRING_TYPE))
        return make_prim_sig(VAL_STRING);

    if (match(TOKEN_CLASS)) {
        if (match(TOKEN_LESS)) {
            consume(TOKEN_IDENTIFIER, "Expect superclass identifier after '<'.");
            return make_class_sig(name); //TODO: add superclassses here later
        }
        return make_class_sig(name);
    }

    if (match(TOKEN_IDENTIFIER)) {
        return make_identifier_sig(parser.previous);
    }

    return make_prim_sig(VAL_NIL);
}

static struct Node* var_declaration(bool require_assign) {
    match(TOKEN_IDENTIFIER);
    Token name = parser.previous;
    consume(TOKEN_COLON, "Expect ':' after identifier.");
    struct Sig* sig = read_sig(name);

    if (sig->type == SIG_IDENTIFIER) {
        consume(TOKEN_EQUAL, "Expect '=' after class name.");
        consume(TOKEN_IDENTIFIER, "Expect class constructor after '='.");
        Token def_klass = parser.previous;
        consume(TOKEN_LEFT_PAREN, "Expect '(' after class name.");
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after '('.");

        Token decl_identifier = ((struct SigIdentifier*)sig)->identifier;

        return make_inst_class(name, decl_identifier, def_klass);
    }

    if (sig->type == SIG_CLASS) {
        consume(TOKEN_EQUAL, "Expect '=' after class declaration.");
        struct Sig* right_sig = read_sig(name);
        consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
        NodeList nl;
        init_node_list(&nl);
        while (!match(TOKEN_RIGHT_BRACE)) {
            //add class signature entries to table here
            struct Node* decl = var_declaration(true);
            struct Sig* prop_sig;
            const char* prop_id_chars;
            int prop_id_length;
            switch (decl->type) {
                case NODE_DECL_VAR:
                    DeclVar* dv = (DeclVar*)decl;
                    prop_sig = dv->sig;
                    prop_id_chars = dv->name.start;
                    prop_id_length = dv->name.length;
                    break;
                case NODE_DECL_FUN:
                    DeclFun* df = (DeclFun*)decl;
                    prop_sig = df->sig;
                    prop_id_chars = df->name.start;
                    prop_id_length = df->name.length;
                    break;
                case NODE_DECL_CLASS:
                    DeclClass* dc = (DeclClass*)decl;
                    prop_sig = dc->sig;
                    prop_id_chars = dc->name.start;
                    prop_id_length = dc->name.length;
                    break;
                default:
                    add_error(name, "Only primitive, function or class definitions allowed in class body.");
                    return NULL;
            }
            struct ObjString* name = make_string(prop_id_chars, prop_id_length);
            push_root(to_string(name));
            set_table(&((struct SigClass*)sig)->props, name, to_sig(prop_sig));
            pop_root();
            add_node(&nl, decl);
        }
        return make_decl_class(name, nl, sig);
    }

    if (sig->type == SIG_FUN) {
        consume(TOKEN_EQUAL, "Expect '=' after variable declaration.");

        if (peek_one(TOKEN_IDENTIFIER)) {
            return make_decl_var(name, sig, expression());
        }

        consume(TOKEN_LEFT_PAREN, "Expect '(' before parameters.");
        NodeList params;
        init_node_list(&params);

        struct Sig* param_sig = make_list_sig();
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                struct Node* var_decl = var_declaration(false);
                DeclVar* vd = (DeclVar*)var_decl;
                add_sig((struct SigList*)param_sig, vd->sig);
                add_node(&params, var_decl);
            } while (match(TOKEN_COMMA));
            consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
        }

        consume(TOKEN_RIGHT_ARROW, "Expect '->' after parameters.");

        struct Sig* ret_sig = read_sig(name);

        consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
        struct Node* body = block();
        struct Sig* fun_sig = make_fun_sig(param_sig, ret_sig);
        if (!same_sig(fun_sig, sig)) {
            add_error(name, "Function declaration type must match definition type.");
        }
        return make_decl_fun(name, params, fun_sig, body);
    }

    if (require_assign) {
        consume(TOKEN_EQUAL, "Expect '=' after variable declaration.");
        struct Node* value = expression();
        return make_decl_var(name, sig, value);
    }

    if (match(TOKEN_EQUAL)) {
        struct Node* value = expression();
        return make_decl_var(name, sig, value);
    } else {
        return make_decl_var(name, sig, NULL); 
    }
}

static struct Node* declaration() {
    if (match(TOKEN_PRINT)) {
        Token name = parser.previous;
        return make_print(name, expression());
    } else if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON)) {
        return var_declaration(true);
    } else if (peek_two(TOKEN_IDENTIFIER, TOKEN_LEFT_PAREN)) {
        struct Node* ce = call_expression();
        if (peek_one(TOKEN_LEFT_PAREN)) {
            return ce;
        }
        return make_expr_stmt(ce);
        /*
    } else if (peek_one(TOKEN_LEFT_PAREN)) {
        struct Node* ce = cascade_expression();
        if (peek_one(TOKEN_LEFT_PAREN)) {
            return ce;
        }
        return make_expr_stmt(ce);*/
    } else if (match(TOKEN_LEFT_BRACE)) {
        return block();
    } else if (match(TOKEN_IF)) {
        Token name = parser.previous;
        struct Node* condition = expression();
        consume(TOKEN_LEFT_BRACE, "Expect '{' after condition.");
        struct Node* then_block = block();
        struct Node* else_block = NULL;
        if (match(TOKEN_ELSE)) {
            consume(TOKEN_LEFT_BRACE, "Expect '{' after else.");
            else_block = block();
        }
        return make_if_else(name, condition, then_block, else_block);
    } else if (match(TOKEN_WHILE)) {
        Token name = parser.previous;
        struct Node* condition = expression();
        consume(TOKEN_LEFT_BRACE, "Expect '{' after condition.");
        struct Node* then_block = block();
        return make_while(name, condition, then_block);
    } else if (match(TOKEN_FOR)) {
        Token name = parser.previous;
        struct Node* initializer = NULL;
        if (!match(TOKEN_COMMA)) {
            initializer = declaration();
            consume(TOKEN_COMMA, "Expect ',' after initializer.");
        }

        struct Node* condition = expression();
        consume(TOKEN_COMMA, "Expect ',' after condition.");

        struct Node* update = NULL;
        if (!match(TOKEN_LEFT_BRACE)) {
            update = expression();
            consume(TOKEN_LEFT_BRACE, "Expect '{' after update.");
        }

        struct Node* then_block = block();

        return make_for(name, initializer, condition, update, then_block);
    } else if (match(TOKEN_RIGHT_ARROW)) {
        Token name = parser.previous;
        struct Node* right = expression(); //NULL if '}'
        return make_return(name, right);
    }

    return make_expr_stmt(expression());
}

static void add_error(Token token, const char* message) {
    ParseError error;
    error.token = token;
    error.message = message;
    parser.errors[parser.error_count] = error;
    parser.error_count++;
}

static void init_parser(const char* source) {
    init_lexer(source);
    parser.error_count = 0;
    parser.current = next_token();
    parser.next = next_token();
    parser.next_next = next_token();
}


ResultCode parse(const char* source, NodeList* nl) {
    init_parser(source);

    while(parser.current.type != TOKEN_EOF) {
        add_node(nl, declaration());
    }

    if (parser.error_count > 0) {
        for (int i = 0; i < parser.error_count; i++) {
            printf("[line %d] %s\n", parser.errors[i].token.line, parser.errors[i].message);
        }
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}

