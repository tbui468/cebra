#include "parser.h"

Parser parser;
Lexer lexer;


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

void parse(char* source) {
    lexer.source = source;
    lexer.start = 0;

    //TODO: remove later.  Here to test parsing
    while (true) {
        Token token = next_token();
        print_token(token);
        if (token.type == TOKEN_EOF) break;
    }

    //scan on demand and parse into AST
    //return AST (which can be a list of sub ASTs)
//    return parse_precedence(parser, PREC_ASSIGNMENT);
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

