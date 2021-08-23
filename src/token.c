#include <stdio.h>
#include "token.h"

void print_token(Token token) {
    switch(token.type) {
        case TOKEN_PLUS: printf("TOKEN_PLUS"); break;
        case TOKEN_MINUS: printf("TOKEN_MINUS"); break;
        case TOKEN_STAR: printf("TOKEN_STAR"); break;
        case TOKEN_SLASH: printf("TOKEN_SLASH"); break;
        case TOKEN_PRINT: printf("TOKEN_PRINT"); break;
        case TOKEN_IDENTIFIER: printf("TOKEN_IDENTIFIER"); break;
        case TOKEN_LEFT_PAREN: printf("TOKEN_LEFT_PAREN"); break;
        case TOKEN_RIGHT_PAREN: printf("TOKEN_RIGHT_PAREN"); break;
        case TOKEN_LEFT_BRACE: printf("TOKEN_LEFT_BRACE"); break;
        case TOKEN_RIGHT_BRACE: printf("TOKEN_RIGHT_BRACE"); break;
        case TOKEN_INT: printf("%s [%.*s]", "TOKEN_INT", token.length, token.start); break;
        case TOKEN_FLOAT: printf("TOKEN_FLOAT"); break;
        case TOKEN_STRING: printf("TOKEN_STRING"); break;
        case TOKEN_CLASS: printf("TOKEN_CLASS"); break;
        case TOKEN_EOF: printf("TOKEN_EOF"); break;
        default: printf("Unrecognized token"); break;

    }
    printf("\n");
}

Token copy_token(Token token) {
    Token copy;
    copy.type = token.type;
    copy.line = token.line;
    copy.start = token.start;
    copy.length = token.length;
    return copy;
}
