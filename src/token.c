#include <stdio.h>
#include <string.h>
#include "token.h"

void print_token(Token token) {
    switch(token.type) {
        case TOKEN_INT: printf("%s [%.*s]", "TOKEN_INT", token.length, token.start); break;
        case TOKEN_FLOAT: printf("TOKEN_FLOAT"); break;
        case TOKEN_STRING: printf("TOKEN_STRING"); break;
        case TOKEN_INT_TYPE: printf("TOKEN_INT_TYPE"); break;
        case TOKEN_FLOAT_TYPE: printf("TOKEN_FLOAT_TYPE"); break;
        case TOKEN_STRING_TYPE: printf("TOKEN_STRING_TYPE"); break;
        case TOKEN_BOOL_TYPE: printf("TOKEN_BOOL_TYPE"); break;
        case TOKEN_NIL_TYPE: printf("TOKEN_NIL_TYPE"); break;
        case TOKEN_TRUE: printf("TOKEN_TRUE"); break;
        case TOKEN_FALSE: printf("TOKEN_FALSE"); break;
        case TOKEN_BANG: printf("TOKEN_BANG"); break;
        case TOKEN_BANG_EQUAL: printf("TOKEN_BANG_EQUAL"); break;
        case TOKEN_EQUAL_EQUAL: printf("TOKEN_EQUAL_EQUAL"); break;
        case TOKEN_LESS: printf("TOKEN_LESS"); break;
        case TOKEN_LESS_EQUAL: printf("TOKEN_LESS_EQUAL"); break;
        case TOKEN_GREATER: printf("TOKEN_GREATER"); break;
        case TOKEN_GREATER_EQUAL: printf("TOKEN_GREATER_EQUAL"); break;
        case TOKEN_IDENTIFIER: printf("TOKEN_IDENTIFIER [%.*s]", token.length, token.start); break;
        case TOKEN_PLUS: printf("TOKEN_PLUS"); break;
        case TOKEN_MINUS: printf("TOKEN_MINUS"); break;
        case TOKEN_STAR: printf("TOKEN_STAR"); break;
        case TOKEN_SLASH: printf("TOKEN_SLASH"); break;
        case TOKEN_MOD: printf("TOKEN_MOD"); break;
        case TOKEN_LEFT_PAREN: printf("TOKEN_LEFT_PAREN"); break;
        case TOKEN_RIGHT_PAREN: printf("TOKEN_RIGHT_PAREN"); break;
        case TOKEN_LEFT_BRACE: printf("TOKEN_LEFT_BRACE"); break;
        case TOKEN_RIGHT_BRACE: printf("TOKEN_RIGHT_BRACE"); break;
        case TOKEN_COLON: printf("TOKEN_COLON"); break;
        case TOKEN_COLON_COLON: printf("TOKEN_COLON_COLON"); break;
        case TOKEN_RIGHT_ARROW: printf("TOKEN_RIGHT_ARROW"); break;
        case TOKEN_EQUAL: printf("TOKEN_EQUAL"); break;
        case TOKEN_IF: printf("TOKEN_IF"); break;
        case TOKEN_ELSE: printf("TOKEN_ELSE"); break;
        case TOKEN_AND: printf("TOKEN_AND"); break;
        case TOKEN_OR: printf("TOKEN_OR"); break;
        case TOKEN_WHILE: printf("TOKEN_WHILE"); break;
        case TOKEN_FOR: printf("TOKEN_FOR"); break;
        case TOKEN_COMMA: printf("TOKEN_COMMA"); break;
        case TOKEN_CLASS: printf("TOKEN_CLASS"); break;
        case TOKEN_DOT: printf("TOKEN_DOT"); break;
        case TOKEN_DUMMY: printf("TOKEN_DUMMY"); break;
        case TOKEN_LIST: printf("TOKEN_LIST"); break;
        case TOKEN_LEFT_BRACKET: printf("TOKEN_LEFT_BRACKET"); break;
        case TOKEN_RIGHT_BRACKET: printf("TOKEN_RIGHT_BRACKET"); break;
        case TOKEN_IN: printf("TOKEN_IN"); break;
        case TOKEN_MAP: printf("TOKEN_MAP"); break;
        case TOKEN_EOF: printf("TOKEN_EOF"); break;
        case TOKEN_NIL: printf("TOKEN_NIL"); break;
        default: printf("Unrecognized token"); break;
    }
    printf("\n");
}

Token make_dummy_token() {
    Token dummy;
    dummy.type = TOKEN_DUMMY;
    dummy.line = 0;
    dummy.start = NULL;
    dummy.length = 0;
    return dummy;
}

Token make_artificial_token(const char* name) {
    Token art;
    art.type = TOKEN_DUMMY;
    art.line = 0;
    art.start = name;
    art.length = strlen(name);
    return art;
}

Token copy_token(Token token) {
    Token copy;
    copy.type = token.type;
    copy.line = token.line;
    copy.start = token.start;
    copy.length = token.length;
    return copy;
}
