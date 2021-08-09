#include "common.h"
#include "lexer.h"

Lexer lexer;


char next_char() {
    return lexer.source[lexer.current++];
}


static bool is_numeric(char c) {
    return c <= '9' && c >= '0';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z'); 
}

static bool is_alpha_numeric(char c) {
    return is_alpha(c) || is_numeric(c) || c == '_';
}

static bool is_whitespace(char c) {
    if (c == '\n') {
        lexer.line++;
    }
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
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
        next_char();
    }
}

static void read_identifier() {
    while (is_alpha_numeric(peek_char())) {
        next_char();
    }
}

static bool match_string(const char* str) {
    if ((int)strlen(str) != lexer.current - lexer.start - 1) {
        return false;
    }

    return memcmp(str, lexer.source + lexer.start + 1, strlen(str)) == 0;
}

static Token read_keyword(char c) {
    switch(c) {
        case 'p':
            read_identifier();
            if (match_string("rint")) {
                return new_token(TOKEN_PRINT);
            } else {
                return new_token(TOKEN_IDENTIFIER);
            }
        default:
            read_identifier();
            return new_token(TOKEN_IDENTIFIER);
    }
}

Token next_token() {
    consume_whitespace();

    char c = next_char();
   
    //float with leading . 
    if (c == '.' && is_numeric(peek_char())) {
        read_numbers();
        return new_token(TOKEN_FLOAT);
    }

    //int or float (with possible trailing .)
    if (is_numeric(c)) {
        read_numbers();
        if (peek_char() == '.') {
            next_char();
        } else {
            return new_token(TOKEN_INT);
        }
        read_numbers();
        return new_token(TOKEN_FLOAT);
    }

    //string literal
    if (c == '"') {
        while (peek_char() != '"') {
            next_char();
        }
        lexer.start++; //skip first double quote
        Token token = new_token(TOKEN_STRING);
        //skip lat double quote
        lexer.start++;
        lexer.current++;
        return token;
    }

    //keyword
    if (is_alpha(c)) {
        return read_keyword(c);
    }

    switch(c) { //current is now on next characters
        case '+':   return new_token(TOKEN_PLUS);
        case '-':   return new_token(TOKEN_MINUS);
        case '*':   return new_token(TOKEN_STAR);
        case '/':   return new_token(TOKEN_SLASH);
        case '(':   return new_token(TOKEN_LEFT_PAREN);
        case ')':   return new_token(TOKEN_RIGHT_PAREN);
        case '\0':  return new_token(TOKEN_EOF);
    }
}

bool end_of_file() {
    return peek_char() == '\0';
}

void init_lexer(const char* source) {
    lexer.source = source;
    lexer.start = 0;
    lexer.current = 0;
    lexer.line = 1;
}