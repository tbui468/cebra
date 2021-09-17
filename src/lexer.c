#include "common.h"
#include "lexer.h"

Lexer lexer;


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

char next_char() {
    return lexer.source[lexer.current++];
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
    read_identifier();

    switch(c) {
        case 'a':
            if (match_string("nd")) return new_token(TOKEN_AND);
            break;
        case 'b':
            if (match_string("ool")) return new_token(TOKEN_BOOL_TYPE);
            break;
        case 'e':
            if (match_string("lse")) return new_token(TOKEN_ELSE);
            break;
        case 'f':
            if (match_string("or")) return new_token(TOKEN_FOR);
            if (match_string("loat")) return new_token(TOKEN_FLOAT_TYPE);
            if (match_string("alse")) return new_token(TOKEN_FALSE);
            break;
        case 'i':
            if (match_string("nt")) return new_token(TOKEN_INT_TYPE);
            if (match_string("f")) return new_token(TOKEN_IF);
            break;
        case 'o':
            if (match_string("r")) return new_token(TOKEN_OR);
            break;
        case 'p':
            if (match_string("rint")) return new_token(TOKEN_PRINT);
            break;
        case 's':
            if (match_string("tring")) return new_token(TOKEN_STRING_TYPE);
            if (match_string("truct")) return new_token(TOKEN_CLASS);
            break;
        case 't':
            if (match_string("rue")) return new_token(TOKEN_TRUE);
            break;
        case 'w':
            if (match_string("hile")) return new_token(TOKEN_WHILE);
            break;
    }
    return new_token(TOKEN_IDENTIFIER);
}

Token next_token() {

    consume_whitespace();

    char c = next_char();

    //skip line comments
    while (c == '/' && peek_char() == '/') {
        while (next_char() != '\n') {
        }        
        lexer.line++;
        consume_whitespace();
        c = next_char();
    }
   
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
        case '-':   
            if (peek_char() == '>') {
                next_char();
                return new_token(TOKEN_RIGHT_ARROW);
            }
            return new_token(TOKEN_MINUS);
        case '*':   return new_token(TOKEN_STAR);
        case '/':   return new_token(TOKEN_SLASH);
        case '%':   return new_token(TOKEN_MOD);
        case '(':   return new_token(TOKEN_LEFT_PAREN);
        case ')':   return new_token(TOKEN_RIGHT_PAREN);
        case '{':   return new_token(TOKEN_LEFT_BRACE);
        case '}':   return new_token(TOKEN_RIGHT_BRACE);
        case ',':   return new_token(TOKEN_COMMA);
        case '.':   return new_token(TOKEN_DOT);
        case ':':   
            if (peek_char() == ':') {
                next_char();
                return new_token(TOKEN_COLON_COLON);
            }
            return new_token(TOKEN_COLON);
        case '=':   
            if (peek_char() == '=') {
                next_char();
                return new_token(TOKEN_EQUAL_EQUAL);
            }
            return new_token(TOKEN_EQUAL);
        case '!':  
            if (peek_char() == '=') {
                next_char();
                return new_token(TOKEN_BANG_EQUAL);
            }
            return new_token(TOKEN_BANG);
        case '<':
            if (peek_char() == '=') {
                next_char();
                return new_token(TOKEN_LESS_EQUAL);
            }
            return new_token(TOKEN_LESS);
        case '>':
            if (peek_char() == '=') {
                next_char();
                return new_token(TOKEN_GREATER_EQUAL);
            }
            return new_token(TOKEN_GREATER);
        case '\0':
            return new_token(TOKEN_EOF);
    }
}

void init_lexer(const char* source) {
    lexer.source = source;
    lexer.start = 0;
    lexer.current = 0;
    lexer.line = 1;
}
