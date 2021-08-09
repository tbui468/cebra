#include "parser.h"
#include "ast.h"
#include "lexer.h"

Parser parser;

static Expr* expression();
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

static Expr* primary() {
    if (match(TOKEN_INT) || match(TOKEN_FLOAT) || match(TOKEN_STRING)) {
        return make_literal(parser.previous);
    } else if (match(TOKEN_LEFT_PAREN)) {
        Expr* expr = expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    }

    add_error(parser.previous, "Unrecognized token.");
    return NULL;
}

static Expr* unary() {
    if (match(TOKEN_MINUS)) {
        Token name = parser.previous;
        Expr* value = unary();
        return make_unary(name, value);
    }

    return primary();
}

static Expr* factor() {
    Expr* left = unary();

    while (match(TOKEN_STAR) || match(TOKEN_SLASH)) {
        Token name = parser.previous;
        Expr* right = unary();
        left = make_binary(name, left, right);
    }

    return left;
}

static Expr* term() {
    Expr* left = factor();

    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        Token name = parser.previous;
        Expr* right = factor();
        left = make_binary(name, left, right);
    }

    return left;
}

static Expr* expression() {
    return term();
}

static Expr* statement() {
    if (match(TOKEN_PRINT)) {
        Token name = parser.previous;
        return make_print(name, expression());
    }

}

static void add_error(Token token, const char* message) {
    ParseError error;
    error.token = token;
    error.message = message;
    parser.errors[parser.error_count] = error;
    parser.error_count++;
}


ResultCode parse(char* source, DeclList* dl) {
    init_lexer(source);
    parser.error_count = 0;

    //add_statement(dl, statement());
    //add_statement(dl, statement());
    int count = 0;
    while(1) {
        print_token(next_token());
        count++;
        if (count > 10) break;
    }

    

    if (parser.error_count > 0) {
        for (int i = 0; i < parser.error_count; i++) {
            printf("[line %d] %s\n", parser.errors[i].token.line, parser.errors[i].message);
        }
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}

