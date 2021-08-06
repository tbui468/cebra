#include "parser.h"

Parser parser;
Lexer lexer;
static void parse_precedence(Precedence prec);

static bool is_numeric(char c) {
    return c <= '9' && c >= '0';
}

static bool is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t';
}

static Token next_token() {

    while (is_whitespace(lexer.source[lexer.start])) {
        lexer.start++;
    }

    int length = 1;
    TokenType type;
    switch(lexer.source[lexer.start]) {
        case '+':   type = TOKEN_PLUS; break;
        case '-':   type = TOKEN_MINUS; break;
        case '*':   type = TOKEN_STAR; break;
        case '/':   type = TOKEN_SLASH; break;
        case '(':   type = TOKEN_LEFT_PAREN; break;
        case ')':   type = TOKEN_RIGHT_PAREN; break;
        case '\0':  type = TOKEN_EOF; break;
        default:
            type = TOKEN_INT;
            while (is_numeric(lexer.source[lexer.start + length])) {
                length++;
            }
            break;
    }

    Token token;
    token.type = type;
    token.start = &lexer.source[lexer.start];
    token.length = length;

    lexer.start += length;

    return token;
}

static void advance() {
    parser.previous = parser.current;
    parser.current = next_token();
}

static void consume(TokenType type, const char* message) {
    if (type == parser.current.type) {
        advance();
        return;
    }

    printf("%s", message);
}

//returns a literal
static void number() {
    printf("number\n");
}

//returns a unary
static void unary() {
    printf("unary\n");
    parse_precedence(PREC_UNARY);
}

static void binary() {
    printf("binary\n");
}

static void grouping() {
    printf("(");
    parse_precedence(PREC_TERM);
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
    printf(")");
}

Precedence get_prec(Token token) {
    switch(token.type) {
        case TOKEN_INT: return PREC_LITERAL;
        case TOKEN_PLUS: return PREC_TERM;
        case TOKEN_MINUS: return PREC_TERM;
        case TOKEN_STAR: return PREC_FACTOR;
        case TOKEN_SLASH: return PREC_FACTOR;
        case TOKEN_EOF: return PREC_NONE;
    }
}

static void parse_precedence(Precedence prec) {
    advance();
    //prefix
    switch(parser.previous.type) {
        case TOKEN_LEFT_PAREN:  grouping(); break;
        case TOKEN_INT:  number(); break;
        case TOKEN_MINUS:   unary(); break;
        default: printf("you fucked up\n"); break;
    }

    //infix
    if (prec <= get_prec(parser.current)) {
        advance();
        switch(parser.previous.type) {
            case TOKEN_PLUS: binary(); break;
            case TOKEN_MINUS: binary(); break;
            case TOKEN_STAR: binary(); break;
            case TOKEN_SLASH: binary(); break;
            default: printf("you fucked up again.\n"); break;
        }
    }
}

void parse(char* source) {
    lexer.source = source;
    lexer.start = 0;
    parser.current = next_token();

/*
    //TODO: remove later.  Here to test parsing
    while (true) {
        Token token = read_token();
        print_token(token);
        if (token.type == TOKEN_EOF) break;
    }*/
    while (parser.current.type != TOKEN_EOF) {
        parse_precedence(PREC_TERM);
    }
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

