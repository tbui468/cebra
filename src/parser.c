#include "parser.h"
#include "ast.h"
#include "lexer.h"

Parser parser;

static struct Node* expression();
static struct Node* declaration();
static void add_error(Token token, const char* message);

static bool match(TokenType type) {
    if (parser.current.type == type) {
        parser.previous = parser.current;
        parser.current = parser.next;
        parser.next = next_token();
        return true;
    }

    return false;
}

static void consume(TokenType type, const char* message) {
    if (type == parser.current.type) {
        parser.previous = parser.current;
        parser.current = parser.next;
        parser.next = next_token();
        return;
    }
    
    add_error(parser.previous, message);
}

static bool peek(TokenType type) {
    return parser.current.type == type;
}

static bool peek_two(TokenType type1, TokenType type2) {
    return parser.current.type == type1 && parser.next.type == type2;
}


static struct Node* primary() {
    if (match(TOKEN_INT) || match(TOKEN_FLOAT) || 
        match(TOKEN_STRING) || match(TOKEN_TRUE) ||
        match(TOKEN_FALSE)) {
        return make_literal(parser.previous);
    } else if (match(TOKEN_IDENTIFIER)) {
        Token name = parser.previous;
        if (match(TOKEN_EQUAL)) {
            return make_set_var(name, expression(), false);
        } else if (match(TOKEN_LEFT_PAREN)) {
            NodeList args;
            init_node_list(&args);
            while (!match(TOKEN_RIGHT_PAREN)) {
                add_node(&args, expression()); 
            }
            return make_call(name, args);
        }
        return make_get_var(name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        struct Node* expr = expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    }

//    add_error(parser.previous, "Unrecognized token.");
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

static Sig* read_sig() {
    if (match(TOKEN_LEFT_PAREN)) {
        SigList params;
        init_sig_list(&params);
        do {
            add_sig(&params, read_sig());
        } while(match(TOKEN_COMMA));
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameter types.");
        consume(TOKEN_RIGHT_ARROW, "Expect '->' followed by return type.");
        Sig* ret = read_sig();
        return make_fun_sig(params, ret);
    }

    if (match(TOKEN_INT_TYPE) ||
        match(TOKEN_FLOAT_TYPE) ||
        match(TOKEN_STRING_TYPE) ||
        match(TOKEN_BOOL_TYPE)) {
        
        return make_prim_sig(get_value_type(parser.previous));
    }

    return make_prim_sig(VAL_NIL);
}

static struct Node* var_declaration(bool require_assign) {
    match(TOKEN_IDENTIFIER);
    Token name = parser.previous;
    consume(TOKEN_COLON, "Expect ':' after identifier.");
    Sig* sig = read_sig();

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
        return make_set_var(name, expression(), true);
    } else if (peek_two(TOKEN_IDENTIFIER, TOKEN_LEFT_PAREN)) {
        match(TOKEN_IDENTIFIER);
        Token name = parser.previous;
        match(TOKEN_LEFT_PAREN);
        NodeList args;
        init_node_list(&args);
        while (!match(TOKEN_RIGHT_PAREN)) {
            add_node(&args, expression()); 
        }
        return make_call(name, args);
    } else if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON_COLON)) {
        match(TOKEN_IDENTIFIER);
        Token name = parser.previous;
        match(TOKEN_COLON_COLON);
        consume(TOKEN_LEFT_PAREN, "Expect '(' before parameters.");
        NodeList params;
        init_node_list(&params);

        SigList sl;
        init_sig_list(&sl); 
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                struct Node* var_decl = var_declaration(false);
                DeclVar* vd = (DeclVar*)var_decl;
                Sig* prim_sig = make_prim_sig(((SigPrim*)vd->sig)->type);
                add_sig(&sl, prim_sig);
                add_node(&params, var_decl);
            } while (match(TOKEN_COMMA));
            consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
        }

        consume(TOKEN_RIGHT_ARROW, "Expect '->' after parameters.");

        Sig* ret_sig = read_sig();

        consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
        struct Node* body = block();
        Sig* sig = make_fun_sig(sl, ret_sig);
        return make_decl_fun(name, params, sig, body);
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

