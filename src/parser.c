#include "parser.h"
#include "ast.h"
#include "lexer.h"

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
    match(TOKEN_LEFT_PAREN);
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
    Token name = parser.previous;
    NodeList args = argument_list();
    struct Node* call = make_call(name, args);

    while (peek_one(TOKEN_LEFT_PAREN)) {
        //TODO: parser.previous really should be the left paren
        call = make_cascade_call(parser.previous, call, argument_list());
    }

    return call;
}

static struct Node* primary() {
    if (match(TOKEN_INT) || match(TOKEN_FLOAT) || 
        match(TOKEN_STRING) || match(TOKEN_TRUE) ||
        match(TOKEN_FALSE)) {
        return make_literal(parser.previous);
    } else if (peek_two(TOKEN_IDENTIFIER, TOKEN_EQUAL)) {
        match(TOKEN_IDENTIFIER);
        Token name = parser.previous;
        match(TOKEN_EQUAL);
        return make_set_var(name, expression());
    } else if (peek_two(TOKEN_IDENTIFIER, TOKEN_LEFT_PAREN)) {
        return call_expression();
    } else if (peek_one(TOKEN_IDENTIFIER)) {
        match(TOKEN_IDENTIFIER);
        Token name = parser.previous;
        return make_get_var(name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        struct Node* expr = expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    }

    return NULL;
}

static struct Node* unary() {
    if (match(TOKEN_MINUS) || match(TOKEN_BANG)) {
        Token name = parser.previous;
        struct Node* value = unary();
        return make_unary(name, value);
    }

    return primary();
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

static struct Node* expression() {
    return or();
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

static struct Sig* read_sig() {
    if (match(TOKEN_LEFT_PAREN)) {
        SigList params;
        init_sig_list(&params);
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                add_sig(&params, read_sig());
            } while(match(TOKEN_COMMA));
            consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameter types.");
        }
        consume(TOKEN_RIGHT_ARROW, "Expect '->' followed by return type.");
        struct Sig* ret = read_sig();
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
            return make_class_sig(parser.previous);
        }
        return make_class_sig(parser.previous);
    }

    return make_prim_sig(VAL_NIL);
}

//TODO: class constructor arguments go here.  Temporary stub
static struct Node* instantiate_class() {
    consume(TOKEN_IDENTIFIER, "Expect class constructor after '='.");
    Token klass = parser.previous;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after class name.");
    consume(TOKEN_LEFT_PAREN, "Expect ')' after '('.");

    return NULL;
}

static struct Node* var_declaration(bool require_assign) {
    match(TOKEN_IDENTIFIER);
    Token name = parser.previous;
    consume(TOKEN_COLON, "Expect ':' after identifier.");
    struct Sig* sig = read_sig();

    if (sig->type == SIG_CLASS) {
        consume(TOKEN_EQUAL, "Expect '=' after class declaration.");
        struct Sig* right_sig = read_sig();
        free(right_sig);
        consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
        NodeList nl;
        init_node_list(&nl);
        while (!match(TOKEN_RIGHT_BRACE)) {
            add_node(&nl, declaration());
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

        SigList param_sig;
        init_sig_list(&param_sig); 
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                struct Node* var_decl = var_declaration(false);
                DeclVar* vd = (DeclVar*)var_decl;
                struct Sig* prim_sig = make_prim_sig(((SigPrim*)vd->sig)->type);
                add_sig(&param_sig, prim_sig);
                add_node(&params, var_decl);
            } while (match(TOKEN_COMMA));
            consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
        }

        consume(TOKEN_RIGHT_ARROW, "Expect '->' after parameters.");

        struct Sig* ret_sig = read_sig();

        consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
        struct Node* body = block();
        struct Sig* fun_sig = make_fun_sig(param_sig, ret_sig);
        if (!same_sig(fun_sig, sig)) {
            add_error(name, "Function declaration type must match definition type.");
        }
        free_sig(sig); //signature should be same as fun_sig at this point, so redundant
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
    } else if (peek_two(TOKEN_IDENTIFIER, TOKEN_EQUAL)) {
        match(TOKEN_IDENTIFIER);
        Token name = parser.previous;
        match(TOKEN_EQUAL);
        return make_expr_stmt(make_set_var(name, expression()));
    } else if (peek_two(TOKEN_IDENTIFIER, TOKEN_LEFT_PAREN)) {
        return make_expr_stmt(call_expression());
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

    return expression();
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

