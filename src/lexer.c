#include <ctype.h>
#include "common.h"
#include "lexer.h"

Lexer lexer;

static bool is_alpha_numeric(unsigned char c) {
    return isalnum(c) || c == '_';
}

static bool is_whitespace(char c) {
    if (c == '\n') {
        lexer.line++;
    }
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

static unsigned char peek_char() {
    if ((unsigned int)(lexer.current) >= strlen(lexer.source)) return '\0';
    return lexer.source[lexer.current]; 
}


static void consume_whitespace() {
    while (is_whitespace(peek_char())) {
        lexer.current++;
    }
    lexer.start = lexer.current;
}

unsigned char next_char() {
    if ((unsigned int)(lexer.current) >= strlen(lexer.source)) return '\0';
    return lexer.source[lexer.current++];
}

static Token new_token(TokenType type) {
    return make_token(type, lexer.line, &lexer.source[lexer.start], lexer.current - lexer.start);
}

static void read_numbers() {
    while (isdigit(peek_char())) {
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
            if (match_string("s")) return new_token(TOKEN_AS);
            break;
        case 'b':
            if (match_string("ool")) return new_token(TOKEN_BOOL_TYPE);
            if (match_string("yte")) return new_token(TOKEN_BYTE_TYPE);
            break;
        case 'e':
            if (match_string("lse")) return new_token(TOKEN_ELSE);
            if (match_string("num")) return new_token(TOKEN_ENUM);
            break;
        case 'f':
            if (match_string("or")) return new_token(TOKEN_FOR);
            if (match_string("oreach")) return new_token(TOKEN_FOR_EACH);
            if (match_string("loat")) return new_token(TOKEN_FLOAT_TYPE);
            if (match_string("alse")) return new_token(TOKEN_FALSE);
            break;
        case 'F':
            if (match_string("ile")) return new_token(TOKEN_FILE_TYPE);
            break;
        case 'i':
            if (match_string("n")) return new_token(TOKEN_IN);
            if (match_string("nt")) return new_token(TOKEN_INT_TYPE);
            if (match_string("f")) return new_token(TOKEN_IF);
            if (match_string("s")) return new_token(TOKEN_IS);
            if (match_string("mport")) return new_token(TOKEN_IMPORT);
            break;
        case 'L':
            if (match_string("ist")) return new_token(TOKEN_LIST);
            break;
        case 'M':
            if (match_string("ap")) return new_token(TOKEN_MAP);
            break;
        case 'n':
            if (match_string("il")) return new_token(TOKEN_NIL);
            break;
        case 'o':
            if (match_string("r")) return new_token(TOKEN_OR);
            break;
        case 's':
            if (match_string("tring")) return new_token(TOKEN_STRING_TYPE);
            if (match_string("truct")) return new_token(TOKEN_STRUCT);
            break;
        case 't':
            if (match_string("rue")) return new_token(TOKEN_TRUE);
            break;
        case 'w':
            if (match_string("hile")) return new_token(TOKEN_WHILE);
            if (match_string("hen")) return new_token(TOKEN_WHEN);
            break;
        case '_':
            if (match_string("")) return new_token(TOKEN_UNDER_SCORE);
            break;
    }
    return new_token(TOKEN_IDENTIFIER);
}

Token next_token() {
    consume_whitespace();

    unsigned char c = next_char();

    //skip line comments
    while (c == '/' && peek_char() == '/') {
        while (peek_char() != '\n' && peek_char() != '\0') {
            next_char();
        }        
        lexer.line++;
        consume_whitespace();
        c = next_char();
    }

    //float with leading . 
    if (c == '.' && isdigit(peek_char())) {
        read_numbers();
        return new_token(TOKEN_FLOAT);
    }

    //int or float (with possible trailing .)
    if (isdigit(c)) {
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
            if (peek_char() == '\0') return new_token(TOKEN_EOF);
            if (peek_char() == '\\') {
                next_char();
                next_char();
            } else {
                next_char();
            }
        }
        lexer.start++; //skip first double quote
        Token token = new_token(TOKEN_STRING);
        //skip lat double quote
        lexer.start++;
        lexer.current++;
        return token;
    }

    //keyword
    if (isalpha(c) || c == '_') {
        return read_keyword(c);
    }

    switch(c) { //current is now on next characters
        case '+':   
            if (peek_char() == '+') {
                next_char();
                return new_token(TOKEN_PLUS_PLUS);
            }
            return new_token(TOKEN_PLUS);
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
        case '[':   return new_token(TOKEN_LEFT_BRACKET);
        case ']':   return new_token(TOKEN_RIGHT_BRACKET);
        case ',':   return new_token(TOKEN_COMMA);
        case '.':   return new_token(TOKEN_DOT);
        case ':':   
            if (peek_char() == ':') {
                next_char();
                return new_token(TOKEN_COLON_COLON);
            }
            if (peek_char() == '=') {
                next_char();
                return new_token(TOKEN_COLON_EQUAL);
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
        default:
            return new_token(TOKEN_DUMMY);
    }
}

void init_lexer(const char* source) {
    lexer.source = source;
    lexer.start = 0;
    lexer.current = 0;
    lexer.line = 1;
}
