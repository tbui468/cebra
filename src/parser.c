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
        parser.current = next_token();
        return true;
    }

    return false;
}

static void consume(TokenType type, const char* message) {
    if (type == parser.current.type) {
        parser.previous = parser.current;
        parser.current = next_token();
        return;
    }
    
    add_error(parser.previous, message);
}

static bool read_type() {
    if (match(TOKEN_INT_TYPE)) return true; 
    if (match(TOKEN_FLOAT_TYPE)) return true; 
    if (match(TOKEN_STRING_TYPE)) return true; 
    if (match(TOKEN_BOOL_TYPE)) return true; 
    return false;
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
        }
        return make_get_var(name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        struct Node* expr = expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    }

    add_error(parser.previous, "Unrecognized token.");
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

    while (match(TOKEN_STAR) || match(TOKEN_SLASH)) {
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

static struct Node* expression() {
    return term();
}

static struct Node* block() {
    Token name = parser.previous;
    DeclList dl;
    init_decl_list(&dl);
    while (!match(TOKEN_RIGHT_BRACE)) {
        add_decl(&dl, declaration());
    }
    return make_block(name, dl);
}

static struct Node* declaration() {
    if (match(TOKEN_PRINT)) {
        Token name = parser.previous;
        return make_print(name, expression());
    } else if (match(TOKEN_IDENTIFIER)) {
        Token name = parser.previous;
        //need to see if this is declaraction (colon) or just a setter/getter
        if (match(TOKEN_COLON)) {
            if (!read_type()) {
                add_error(parser.previous, "Expect data type after ':'.");
            }
            Token type = parser.previous;
            consume(TOKEN_EQUAL, "Expect '=' after variable declaration.");
            struct Node* value = expression();
            return make_decl_var(name, type, value);
        } else if (match(TOKEN_EQUAL)) {
            return make_set_var(name, expression(), true);
        }
    } else if (match(TOKEN_LEFT_BRACE)) {
        return block();
    } else if (match(TOKEN_IF)) {
        Token name = parser.previous;
        struct Node* condition = expression();
        consume(TOKEN_LEFT_BRACE, "Expect '{' after condition.");
        struct Node* then_block = block();
        struct Node* else_block = NULL;
        if (match(TOKEN_ELSE)) {
            else_block = block();
        }

        return make_if_else(name, condition, then_block, else_block);
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
}


ResultCode parse(const char* source, DeclList* dl) {
    init_parser(source);

    while(parser.current.type != TOKEN_EOF) {
        add_decl(dl, declaration());
    }

    if (parser.error_count > 0) {
        for (int i = 0; i < parser.error_count; i++) {
            printf("[line %d] %s\n", parser.errors[i].token.line, parser.errors[i].message);
        }
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}

