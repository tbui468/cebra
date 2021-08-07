#include "parser.h"
#include "ast.h"

Parser parser;
Lexer lexer;

static Expr* expression();
static void add_error(Token token, const char* message);


static bool is_numeric(char c) {
    return c <= '9' && c >= '0';
}

static bool is_whitespace(char c) {
    if (c == '\n') {
        lexer.line++;
    }
    return c == ' ' || c == '\n' || c == '\t';
}

static char peek_char() {
    return lexer.source[lexer.current]; 
}

static void consume_whitespace() {
    while (is_whitespace(peek_char())) {
        lexer.current++;
    }
    lexer.start = lexer.current;
}

static char get_char() {
    lexer.current++;
    return lexer.source[lexer.current - 1];
}

static Token new_token(TokenType type) {
    Token token;
    token.type = type;
    token.line = lexer.line;
    token.start = &lexer.source[lexer.start];
    token.length = lexer.current - lexer.start;

    lexer.start = lexer.current;

    return token;
}

static void read_numbers() {
    while (is_numeric(peek_char())) {
        get_char();
    }
}

static Token next_token() {

    consume_whitespace();

    switch(get_char()) { //current is now on next characters
        case '+':   return new_token(TOKEN_PLUS);
        case '-':   return new_token(TOKEN_MINUS);
        case '*':   return new_token(TOKEN_STAR);
        case '/':   return new_token(TOKEN_SLASH);
        case '(':   return new_token(TOKEN_LEFT_PAREN);
        case ')':   return new_token(TOKEN_RIGHT_PAREN);
        case '.': {
            read_numbers();
            return new_token(TOKEN_FLOAT);
            break;
        }
        case '\0':  return new_token(TOKEN_EOF);
        default: {
            read_numbers();
            if (peek_char() == '.') {
                get_char();
            } else {
                return new_token(TOKEN_INT);
            }
            read_numbers();
            return new_token(TOKEN_FLOAT);
            break;
        }
    }
}

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
    if (match(TOKEN_INT) || match(TOKEN_FLOAT)) {
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

static void add_error(Token token, const char* message) {
    ParseError error;
    error.token = token;
    error.message = message;
    parser.errors[parser.error_count] = error;
    parser.error_count++;
}



ResultCode parse(char* source, Expr** root) {
    lexer.source = source;
    lexer.start = 0;
    lexer.current = 0;
    lexer.line = 1;
    parser.current = next_token();
    parser.error_count = 0;

    //TODO: Allow more than an single expression
//    while (parser.current.type != TOKEN_EOF) {
    *root = expression();
//    }

    if (parser.error_count > 0) {
        for (int i = 0; i < parser.error_count; i++) {
            printf("[line %d] %s\n", parser.errors[i].token.line, parser.errors[i].message);
        }
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}


void print_token(Token token) {
    switch(token.type) {
        case TOKEN_PLUS: printf("TOKEN_PLUS"); break;
        case TOKEN_MINUS: printf("TOKEN_MINUS"); break;
        case TOKEN_STAR: printf("TOKEN_STAR"); break;
        case TOKEN_SLASH: printf("TOKEN_SLASH"); break;
        case TOKEN_LEFT_PAREN: printf("TOKEN_LEFT_PAREN"); break;
        case TOKEN_RIGHT_PAREN: printf("TOKEN_RIGHT_PAREN"); break;
        case TOKEN_INT: printf("%s [%.*s]", "TOKEN_INT", token.length, token.start); break;
        case TOKEN_EOF: printf("TOKEN_EOF"); break;
        default: printf("Unrecognized token."); break;
    }
    printf("\n");
}

