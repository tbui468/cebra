#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "memory.h"

Parser parser;

static ResultCode expression(Token var_name, struct Node** node);
static ResultCode declaration(struct Node** node);
static void add_error(Token token, const char* message);
static ResultCode block(struct Node* prepend, struct Node** node);
static ResultCode read_sig(Token var_name, struct Sig** sig);
static ResultCode param_declaration(struct Node** node);
static ResultCode var_declaration(struct Node** node);

static void print_all_tokens() {
    printf("******start**********\n");
    printf("%d\n", parser.previous.type);
    printf("%d\n", parser.current.type);
    printf("%d\n", parser.next.type);
    printf("%d\n", parser.next_next.type);
    printf("******end**********\n");
}

static void advance() {
    parser.previous = parser.current;
    parser.current = parser.next;
    parser.next = parser.next_next;
    parser.next_next = next_token();
}

static bool match(TokenType type) {
    if (parser.current.type == type) {
        advance();
        return true;
    }

    return false;
}

static bool peek_one(TokenType type) {
    return parser.current.type == type;
}
static bool peek_two(TokenType type1, TokenType type2) {
    return parser.current.type == type1 && parser.next.type == type2;
}
static bool peek_three(TokenType type1, TokenType type2, TokenType type3) {
    return parser.current.type == type1 && 
           parser.next.type == type2 &&
           parser.next_next.type == type3;
}

static void synchronize() {
    while (!peek_one(TOKEN_EOF)) {
        if (peek_one(TOKEN_FOR)) return;
        if (peek_one(TOKEN_FOR_EACH)) return;
        if (peek_one(TOKEN_WHILE)) return;
        if (peek_one(TOKEN_IF)) return;
        if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON)) return;
        if (peek_two(TOKEN_IDENTIFIER, TOKEN_LEFT_PAREN)) return;
        advance();
    }
}

static bool consume(TokenType type, const char* message) {
    if (type == parser.current.type) {
        advance();
        return true;
    }
    
    add_error(parser.previous, message);
    return false;
}

/*
static struct NodeList* argument_list(Token var_name) {
    struct NodeList* args = (struct NodeList*)make_node_list();
    if (!match(TOKEN_RIGHT_PAREN)) {
        do {
            struct Node* arg = expression(var_name);
            if (arg == NULL) {
                add_error(parser.previous, "Invalid argument.");
                return NULL;
            }
            add_node(args, arg); 
        } while (match(TOKEN_COMMA));
        if (!consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.")) return NULL;
    }

    return args;
}*/


static ResultCode primary(Token var_name, struct Node** node) {
    if (match(TOKEN_INT) || match(TOKEN_FLOAT) || 
        match(TOKEN_STRING) || match(TOKEN_TRUE) ||
        match(TOKEN_FALSE)) {
        *node = make_literal(parser.previous);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_LIST)) {
        Token identifier = parser.previous;
        if (!consume(TOKEN_LESS, "Expect '<' after 'List'.")) return RESULT_FAILED;
        struct Sig* template_type;
        if (read_sig(var_name, &template_type) == RESULT_FAILED || sig_is_type(template_type, VAL_NIL)) {
            add_error(parser.previous, "List type inside <> must be valid.");
            return RESULT_FAILED;
        }
        if (!consume(TOKEN_GREATER, "Expect '>' after type.")) return RESULT_FAILED;
        *node = make_get_var(identifier, make_list_sig(template_type)); 
        return RESULT_SUCCESS;
    } else if (match(TOKEN_MAP)) {
        Token identifier = parser.previous;
        if (!consume(TOKEN_LESS, "Expect '<' after 'Map'.")) return RESULT_FAILED;
        struct Sig* template_type;
        if (read_sig(var_name, &template_type) == RESULT_FAILED || sig_is_type(template_type, VAL_NIL)) {
            add_error(parser.previous, "Map type inside <> must be valid.");
            return RESULT_FAILED;
        }
        if (!consume(TOKEN_GREATER, "Expect '>' after type.")) return RESULT_FAILED;
        *node = make_get_var(identifier, make_map_sig(template_type)); 
        return RESULT_SUCCESS;
    } else if (match(TOKEN_IDENTIFIER)) {
        *node = make_get_var(parser.previous, NULL);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_STRING_TYPE)) {
        Token token = parser.previous;
        token.type = TOKEN_IDENTIFIER;
        *node = make_get_var(token, NULL);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_LEFT_PAREN)) {
        Token name = parser.previous;
        if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON) ||
            peek_two(TOKEN_RIGHT_PAREN, TOKEN_RIGHT_ARROW)) {
            struct NodeList* params = (struct NodeList*)make_node_list();
            
            struct Sig* param_sig = make_array_sig();
            if (!match(TOKEN_RIGHT_PAREN)) {
                do {
                    struct Node* var_decl;
                    if (param_declaration(&var_decl) == RESULT_FAILED) return RESULT_FAILED;

                    if (match(TOKEN_EQUAL)) {
                        add_error(parser.previous, "Function parameters cannot be defined.");
                        return RESULT_FAILED;
                    }

                    DeclVar* vd = (DeclVar*)var_decl;
                    add_sig((struct SigArray*)param_sig, vd->sig);
                    add_node(params, var_decl);
                } while (match(TOKEN_COMMA));
                if (!consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameter list.")) return RESULT_FAILED;
            }

            if (!consume(TOKEN_RIGHT_ARROW, "Expect '->' after parameter list.")) return RESULT_FAILED;

            struct Sig* ret_sig;
            if (read_sig(var_name, &ret_sig) == RESULT_FAILED) {
                add_error(parser.previous, "Expect valid return type after function parameters.");
                return RESULT_FAILED;
            }

            if (!consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.")) return RESULT_FAILED;

            struct Node* body;
            if (block(NULL, &body) == RESULT_FAILED) {
                add_error(parser.previous, "Expect '}' after function body.");
                return RESULT_FAILED;
            }

            struct Sig* fun_sig = make_fun_sig(param_sig, ret_sig); 

            *node = make_decl_fun(var_name, params, fun_sig, body);
            return RESULT_SUCCESS;
        }

        //check for group (expression in parentheses)
        struct Node* expr;
        if (expression(var_name, &expr) == RESULT_FAILED) {
            add_error(parser.previous, "Expect expression after '('.");
            return RESULT_FAILED;
        }

        if (!consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.")) return RESULT_FAILED;
        *node = expr;
        return RESULT_SUCCESS;
    } else if (peek_one(TOKEN_CLASS)) {
        //this branch is only entered if next is TOKEN_CLASS
        //so read_sig(var_name) will always be valid, so no NULL check
        struct Sig* right_sig;
        if (read_sig(var_name, &right_sig) == RESULT_FAILED) return RESULT_FAILED;

        struct Node* super = parser.previous.type == TOKEN_IDENTIFIER ? 
                             make_get_var(parser.previous, NULL) : 
                             NULL;

        if (!consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.")) return RESULT_FAILED;

        struct NodeList* nl = (struct NodeList*)make_node_list();
        while (!match(TOKEN_RIGHT_BRACE)) {
            struct Node* decl;
            if (var_declaration(&decl) == RESULT_FAILED) return RESULT_FAILED;
            if (decl->type != NODE_DECL_VAR) {
                add_error(var_name, "Only primitive, function or struct definitions allowed in struct body.");
                return RESULT_FAILED;
            }

            add_node(nl, decl);
        }

        *node = make_decl_class(var_name, super, nl);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_NIL)) {
        *node = make_nil(parser.previous);
        return RESULT_SUCCESS;
    }

    //empty return ('->') is same as returning 'nil'
    if (parser.previous.type == TOKEN_RIGHT_ARROW && peek_one(TOKEN_RIGHT_BRACE)) {
        *node = make_nil(parser.previous);
        return RESULT_SUCCESS;
    }

    return RESULT_FAILED;
}

static ResultCode call_dot(Token var_name, struct Node** node) {
    struct Node* left;
    if (primary(var_name, &left) == RESULT_FAILED) {
        switch(parser.previous.type) {
            case TOKEN_DOT:
                add_error(parser.previous, "Left of '.' must be a struct instance.");
                break;
            case TOKEN_LEFT_PAREN:
                add_error(parser.previous, "Left of '(' must be an identifier.");
                break;
            case TOKEN_LEFT_BRACKET:
                add_error(parser.previous, "Left of '[' must be a List or Map identifier.");
                break;
        }
        return RESULT_FAILED;
    }

    while ((match(TOKEN_DOT) || match(TOKEN_LEFT_PAREN) || match(TOKEN_LEFT_BRACKET))) {
        switch(parser.previous.type) {
            case TOKEN_DOT:
                if (!consume(TOKEN_IDENTIFIER, "Expect identifier after '.'.")) return RESULT_FAILED;
                left = make_get_prop(left, parser.previous);
                break;
            case TOKEN_LEFT_PAREN:
                struct NodeList* args = (struct NodeList*)make_node_list();
                if (!match(TOKEN_RIGHT_PAREN)) {
                    do {
                        struct Node* arg;
                        if (expression(var_name, &arg) == RESULT_FAILED) {
                            add_error(parser.previous, "Invalid argument.");
                            return RESULT_FAILED;
                        }
                        add_node(args, arg); 
                    } while (match(TOKEN_COMMA));
                    if (!consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.")) return RESULT_FAILED;
                }

                left = make_call(parser.previous, left, args);
                break;
            case TOKEN_LEFT_BRACKET:
                struct Node* idx;
                if (expression(var_name, &idx) == RESULT_FAILED) {
                    add_error(parser.previous, "Index must be a string for Maps, or integer for Lists.");
                    return RESULT_FAILED;
                }
                left = make_get_idx(parser.previous, left, idx);
                if (!consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.")) return RESULT_FAILED;
                break;
            default:
                break;
        }
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode unary(Token var_name, struct Node** node) {
    if (match(TOKEN_MINUS) || match(TOKEN_BANG)) {
        Token op = parser.previous;
        struct Node* right;
        if (unary(var_name, &right) == RESULT_FAILED) {
            if (op.type == TOKEN_MINUS) {
                add_error(op, "Right side of '-' must be valid expression.");
            } else {
                add_error(op, "Right side of '!' must be valid expression.");
            }
            return RESULT_FAILED;
        }
        *node = make_unary(parser.previous, right);
        return RESULT_SUCCESS;
    }

    return call_dot(var_name, node);
}

static ResultCode factor(Token var_name, struct Node** node) {
    struct Node* left;
    if (unary(var_name, &left) == RESULT_FAILED) {
        switch(parser.current.type) {
            case TOKEN_MOD:
                add_error(parser.previous, "Left hand side of '%' must evaluate to int.");
                break;
            case TOKEN_STAR:
                add_error(parser.previous, "Left hand side of '*' must evaluate to int or float.");
                break;
            case TOKEN_SLASH:
                add_error(parser.previous, "Left hand side of '/' must evaluate to int or float.");
                break;
        }
        return RESULT_FAILED;
    }

    while (match(TOKEN_STAR) || match(TOKEN_SLASH) || match(TOKEN_MOD)) {
        Token name = parser.previous;
        struct Node* right;
        if (unary(var_name, &right) == RESULT_FAILED) {
            switch(parser.previous.type) {
                case TOKEN_MOD:
                    add_error(name, "Right hand side of '%' must evaluate to int.");
                    break;
                case TOKEN_STAR:
                    add_error(name, "Right hand side of '*' must evaluate to int or float.");
                    break;
                case TOKEN_SLASH:
                    add_error(name, "Right hand side of '/' must evaluate to int or float.");
                    break;
            }
            return RESULT_FAILED;
        }
        left = make_binary(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode term(Token var_name, struct Node** node) {
    struct Node* left;
    if (factor(var_name, &left) == RESULT_FAILED) {
        switch(parser.current.type) {
            case TOKEN_PLUS:
                add_error(parser.previous, "Left hand side of '+' must evaluate to int, float or string.");
                break;
            case TOKEN_MINUS:
                add_error(parser.previous, "Left hand side of '-' must evaluate to int or float.");
                break;
        }
        return RESULT_FAILED;
    }

    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        Token name = parser.previous;
        struct Node* right;
        if (factor(var_name, &right) == RESULT_FAILED) {
            switch(parser.previous.type) {
                case TOKEN_PLUS:
                    add_error(name, "Right hand side of '+' must evaluate to int, float or string.");
                    break;
                case TOKEN_MINUS:
                    add_error(name, "Right hand side of '-' must evaluate to int or float.");
                    break;
            }
            return RESULT_FAILED;
        }
        left = make_binary(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode relation(Token var_name, struct Node** node) {
    struct Node* left;
    if (term(var_name, &left) == RESULT_FAILED) {
        switch(parser.current.type) {
            case TOKEN_LESS:
                add_error(parser.previous, "Left hand side of '<' must be an expression.");
                break;
            case TOKEN_LESS_EQUAL:
                add_error(parser.previous, "Left hand side of '<=' must be an expression.");
                break;
            case TOKEN_GREATER:
                add_error(parser.previous, "Left hand side of '>' must be an expression.");
                break;
            case TOKEN_GREATER_EQUAL:
                add_error(parser.previous, "Left hand side of '>=' must be an expression.");
                break;
        }
        return RESULT_FAILED;
    }

    while (match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL) ||
           match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL)) {
        Token name = parser.previous;
        struct Node* right;
        if (term(var_name, &right) == RESULT_FAILED) {
            switch(parser.previous.type) {
                case TOKEN_LESS:
                    add_error(parser.previous, "Right hand side of '<' must be an expression.");
                    break;
                case TOKEN_LESS_EQUAL:
                    add_error(parser.previous, "Right hand side of '<=' must be an expression.");
                    break;
                case TOKEN_GREATER:
                    add_error(parser.previous, "Right hand side of '>' must be an expression.");
                    break;
                case TOKEN_GREATER_EQUAL:
                    add_error(parser.previous, "Right hand side of '>=' must be an expression.");
                    break;
            }
            return RESULT_FAILED;
        }
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode equality(Token var_name, struct Node** node) {
    struct Node* left;
    if (relation(var_name, &left) == RESULT_FAILED) {
        switch(parser.previous.type) {
            case TOKEN_EQUAL_EQUAL:
                add_error(parser.previous, "Left hand side of '==' must be an expression.");
                break;
            case TOKEN_BANG_EQUAL:
                add_error(parser.previous, "Left hand side of '!=' must be an expression.");
                break;
            case TOKEN_IN:
                add_error(parser.previous, "Left hand side of 'in' must be an expression.");
                break;
        }
        return RESULT_FAILED;
    }

    while (match(TOKEN_EQUAL_EQUAL) || match(TOKEN_BANG_EQUAL) || match(TOKEN_IN)) {
        Token name = parser.previous;
        struct Node* right;
        if (relation(var_name, &right) == RESULT_FAILED) {
            switch(parser.previous.type) {
                case TOKEN_EQUAL_EQUAL:
                    add_error(parser.previous, "Right hand side of '==' must be an expression.");
                    break;
                case TOKEN_BANG_EQUAL:
                    add_error(parser.previous, "Right hand side of '!=' must be an expression.");
                    break;
                case TOKEN_IN:
                    add_error(parser.previous, "Right hand side of 'in' must be a List.");
                    break;
            }
            return RESULT_FAILED;
        }
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode and(Token var_name, struct Node** node) {
    struct Node* left;
    if (equality(var_name, &left) == RESULT_FAILED) {
        add_error(parser.previous, "Left hand side of 'and' must evaluate to a boolean.");
        return RESULT_FAILED;
    }

    while (match(TOKEN_AND)) {
        Token name = parser.previous;
        struct Node* right;
        if (equality(var_name, &right) == RESULT_FAILED) {
            add_error(parser.previous, "Right hand side of 'and' must evaluate to a boolean.");
            return RESULT_FAILED;
        }
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode or(Token var_name, struct Node** node) {
    struct Node* left;
    if (and(var_name, &left) == RESULT_FAILED) {
        add_error(parser.previous, "Left hand side of 'or' must evaluate to a boolean.");
        return RESULT_FAILED;
    }

    while (match(TOKEN_OR)) {
        Token name = parser.previous;
        struct Node* right;
        if (and(var_name, &right) == RESULT_FAILED) {
            add_error(parser.previous, "Right hand side of 'or' must evaluate to a boolean.");
            return RESULT_FAILED;
        }
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode assignment(Token var_name, struct Node** node) {
    struct Node* left;
    if (or(var_name, &left) == RESULT_FAILED) {
        add_error(parser.previous, "Left hand side of '=' must be assignable.");
        return RESULT_FAILED;
    }

    while (match(TOKEN_EQUAL)) {
        struct Node* right;
        if (expression(var_name, &right) == RESULT_FAILED) {
            add_error(parser.previous, "Right hand side of '=' must be an expression.");
            return RESULT_FAILED;
        }
        if (left->type == NODE_GET_IDX) {
            left = make_set_idx(left, right);
        } else if (left->type == NODE_GET_VAR) {
            left = make_set_var(left, right);
        } else { //NODE_GET_PROP
            left = make_set_prop(left, right);
        }
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode expression(Token var_name, struct Node** node) {
    return assignment(var_name, node);
}

static ResultCode block(struct Node* prepend, struct Node** node) {
    Token name = parser.previous;
    struct NodeList* body = (struct NodeList*)make_node_list();

    if (prepend != NULL) {
        add_node(body, prepend);
    }

    while (!match(TOKEN_RIGHT_BRACE)) {
        if (peek_one(TOKEN_EOF)) {
            return RESULT_FAILED;
        }
        struct Node* decl;
        if (declaration(&decl) == RESULT_SUCCESS) {
            add_node(body, decl);
        } else {
            synchronize();
        }
    }
    *node = make_block(name, body);
    return RESULT_SUCCESS;
}

static ResultCode read_sig(Token var_name, struct Sig** sig) {
    if (parser.previous.type == TOKEN_COLON && peek_one(TOKEN_EQUAL)) {
        *sig = make_decl_sig();
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_LEFT_PAREN)) {
        struct Sig* params = make_array_sig();
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                struct Sig* param_sig;
                if (read_sig(var_name, &param_sig) == RESULT_FAILED) {
                    add_error(var_name, "Invalid parameter type.");
                    return RESULT_FAILED;
                }
                add_sig((struct SigArray*)params, param_sig);
            } while(match(TOKEN_COMMA));
            if (!consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameter types.")) return RESULT_FAILED;
        }
        if (!consume(TOKEN_RIGHT_ARROW, "Expect '->' followed by return type.")) return RESULT_FAILED;
        struct Sig* ret;
        if (read_sig(var_name, &ret) == RESULT_FAILED) {
            add_error(var_name, "Invalid return type.");
            return RESULT_FAILED;
        }
        *sig = make_fun_sig(params, ret);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_INT_TYPE)) {
        *sig = make_prim_sig(VAL_INT);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_FLOAT_TYPE)) {
        *sig = make_prim_sig(VAL_FLOAT);
        return RESULT_SUCCESS;
    }
    
    if (match(TOKEN_BOOL_TYPE)) {
        *sig = make_prim_sig(VAL_BOOL);
        return RESULT_SUCCESS;
    }
    
    if (match(TOKEN_STRING_TYPE)) {
        *sig = make_prim_sig(VAL_STRING);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_CLASS)) {
        if (match(TOKEN_LESS)) {
            if (!consume(TOKEN_IDENTIFIER, "Expect superclass identifier after '<'.")) return RESULT_FAILED;
            *sig = make_class_sig(var_name);
            return RESULT_SUCCESS;
        }

        *sig = make_class_sig(var_name);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_LIST)) {
        if (!consume(TOKEN_LESS, "Expect '<' after 'List'.")) return RESULT_FAILED;
        struct Sig* template_type;
        if (read_sig(var_name, &template_type) == RESULT_FAILED || sig_is_type(template_type, VAL_NIL)) {
            add_error(parser.previous, "List type inside <> must be valid.");
            return RESULT_FAILED;
        }
        if (!consume(TOKEN_GREATER, "Expect '>' after type.")) return RESULT_FAILED;
        *sig = make_list_sig(template_type);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_MAP)) {
        if (!consume(TOKEN_LESS, "Expect '<' after 'Map'.")) return RESULT_FAILED;
        struct Sig* template_type;
        if (read_sig(var_name, &template_type) == RESULT_FAILED || sig_is_type(template_type, VAL_NIL)) {
            add_error(parser.previous, "Map type inside <> must be valid.");
            return RESULT_FAILED;
        }
        if (!consume(TOKEN_GREATER, "Expect '>' after type.")) return RESULT_FAILED;
        *sig = make_map_sig(template_type);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_IDENTIFIER)) {
        *sig = make_identifier_sig(parser.previous);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_NIL) || 
            //function definition
            (parser.previous.type == TOKEN_RIGHT_ARROW && peek_one(TOKEN_LEFT_BRACE)) ||
            //function declaration
            (parser.previous.type == TOKEN_RIGHT_ARROW && peek_one(TOKEN_EQUAL))) {
        *sig = make_prim_sig(VAL_NIL);
        return RESULT_SUCCESS;
    }

    return RESULT_FAILED;
}

static ResultCode param_declaration(struct Node** node) {
    if (!match(TOKEN_IDENTIFIER)) {
        add_error(parser.previous, "Parameter must begin with identifier.");
        return RESULT_FAILED;
    }
    Token var_name = parser.previous;
    if (!match(TOKEN_COLON)) {
        add_error(var_name, "Parameter identifer must be followed by ':'.");
        return RESULT_FAILED;
    }

    struct Sig* sig;
    if (read_sig(var_name, &sig) == RESULT_FAILED || sig_is_type(sig, VAL_NIL)) {
        add_error(var_name, "Parameter type must be valid.");
        return RESULT_FAILED;
    }

    *node = make_decl_var(var_name, sig, NULL);
    return RESULT_SUCCESS;
}

static ResultCode var_declaration(struct Node** node) {
    match(TOKEN_IDENTIFIER);
    Token var_name = parser.previous;
    match(TOKEN_COLON);  //var_declaration only called if this is true, so no need to check

    struct Sig* sig;
    if (read_sig(var_name, &sig) == RESULT_FAILED) {
        add_error(var_name, "Expecting variable type.");
        return RESULT_FAILED;
    }

    if (!consume(TOKEN_EQUAL, "Variables must be defined at declaration.")) return RESULT_FAILED;

    struct Node* right;
    if (expression(var_name, &right) == RESULT_FAILED) {
        add_error(var_name, "Variable must be assigned to valid expression.");
        return RESULT_FAILED;
    }
    *node = make_decl_var(var_name, sig, right);
    return RESULT_SUCCESS;
}

static ResultCode declaration(struct Node** node) {
    if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON)) {
        return var_declaration(node);
    } else if (match(TOKEN_LEFT_BRACE)) {
        Token name = parser.previous;
        if (block(NULL, node) == RESULT_FAILED) {
            add_error(name, "Expect '}' after block body.");
            return RESULT_FAILED;
        }
        return RESULT_SUCCESS;
    } else if (match(TOKEN_IF)) {
        Token name = parser.previous;

        struct Node* condition;
        if (expression(make_dummy_token(), &condition) == RESULT_FAILED) {
            add_error(name, "Expect boolean expression after 'if'.");
            return RESULT_FAILED;
        }

        if (!consume(TOKEN_LEFT_BRACE, "Expect '{' after 'if' condition.")) return RESULT_FAILED;

        struct Node* then_block;
        if (block(NULL, &then_block) == RESULT_FAILED) {
            add_error(name, "Expect '}' after 'if' statement body.");
            return RESULT_FAILED;
        }

        struct Node* else_block = NULL;
        if (match(TOKEN_ELSE)) {
            if (!consume(TOKEN_LEFT_BRACE, "Expect '{' after 'else'.")) return RESULT_FAILED;

            if (block(NULL, &else_block) == RESULT_FAILED) {
                add_error(name, "Expect '}' after 'else' statement body.");
                return RESULT_FAILED;
            }
        }

        *node = make_if_else(name, condition, then_block, else_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_WHILE)) {
        Token name = parser.previous;
        struct Node* condition;
        if (expression(make_dummy_token(), &condition) == RESULT_FAILED) {
            add_error(name, "Expect boolean expression after 'while'.");
            return RESULT_FAILED;
        }
        if (!consume(TOKEN_LEFT_BRACE, "Expect boolean expression and '{' after 'while'.")) return RESULT_FAILED;

        struct Node* then_block;
        if (block(NULL, &then_block) == RESULT_FAILED) {
            add_error(name, "Expect '}' after 'while' body.");
            return RESULT_FAILED;
        }
        *node = make_while(name, condition, then_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_FOR)) {
        Token name = parser.previous;
        struct Node* initializer = NULL;
        if (!match(TOKEN_COMMA)) {
            if (declaration(&initializer) == RESULT_FAILED) {
                add_error(name, "Expect initializer or empty space for first item in 'for' loop.");
                return RESULT_FAILED;
            }
            if (!consume(TOKEN_COMMA, "Expect ',' after for-loop initializer.")) return RESULT_FAILED;
        }

        struct Node* condition = NULL;
        if (!match(TOKEN_COMMA)) {
            if (expression(make_dummy_token(), &condition) == RESULT_FAILED) {
                add_error(name, "Expect condition or empty space for second item in 'for' loop.");
                return RESULT_FAILED;
            }
            if (!consume(TOKEN_COMMA, "Expect ',' after for-loop condition.")) return RESULT_FAILED;
        }

        struct Node* update = NULL;
        if (!match(TOKEN_LEFT_BRACE)) {
            struct Node* update_expr;
            if (expression(make_dummy_token(), &update_expr) == RESULT_FAILED) {
                add_error(name, "Expect update or empty space for third item in 'for' loop.");
                return RESULT_FAILED;
            }
            update = make_expr_stmt(update_expr);
            if (!consume(TOKEN_LEFT_BRACE, "Expect '{' after for-loop update.")) return RESULT_FAILED;
        }

        struct Node* then_block;
        if (block(NULL, &then_block) == RESULT_FAILED) {
            add_error(name, "Expect '}' after for-loop body.");
            return RESULT_FAILED;
        }

        *node = make_for(name, initializer, condition, update, then_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_FOR_EACH)) {
        Token name = parser.previous;
        struct Node* element;
        if (param_declaration(&element) == RESULT_FAILED) return RESULT_FAILED;

        if (!consume(TOKEN_IN, "Expect 'in' after element declaration.")) return RESULT_FAILED;

        struct Node* list;
        if (expression(make_dummy_token(), &list) == RESULT_FAILED) {
            add_error(name, "Expect List after 'in'.");
            return RESULT_FAILED;
        }

        //initializer
        Token idx_token = make_token(TOKEN_IDENTIFIER, -1, "_idx_", 5);
        Token zero_token = make_token(TOKEN_INT, -1, "0", 1);
        struct Node* initializer = make_decl_var(idx_token, make_prim_sig(VAL_INT), make_literal(zero_token));

        //condition 
        Token prop_token = make_token(TOKEN_IDENTIFIER, -1, "size", 4);
        Token less_token = make_token(TOKEN_LESS, -1, "<", 1);
        struct Node* get_idx = make_get_var(idx_token, NULL);
        struct Node* get_size = make_get_prop(list, prop_token);
        struct Node* condition = make_logical(less_token, get_idx, get_size);

        //update
        Token plus_token = make_token(TOKEN_PLUS, -1, "+", 1);
        Token one_token = make_token(TOKEN_INT, -1, "1", 1);
        struct Node* sum = make_binary(plus_token, get_idx, make_literal(one_token));
        struct Node* update = make_expr_stmt(make_set_var(get_idx, sum)); 

        //body
        if (!consume(TOKEN_LEFT_BRACE, "Expect '{' before foreach loop body.")) return RESULT_FAILED;

        struct Node* get_element = make_get_idx(make_dummy_token(), list, get_idx);
        ((DeclVar*)element)->right = get_element;
        struct Node* then_block;
        if (block(element, &then_block) == RESULT_FAILED) {
            add_error(name, "Expect '}' after for-loop body.");
            return RESULT_FAILED;
        }

        *node = make_for(name, initializer, condition, update, then_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_RIGHT_ARROW)) {
        Token name = parser.previous;
        struct Node* right;
        if (expression(make_dummy_token(), &right) == RESULT_FAILED) {
            add_error(name, "Returned value must be expression, or empty (for nil return).");
            return RESULT_FAILED;
        }
        *node = make_return(name, right);
        return RESULT_SUCCESS;
    }

    struct Node* expr;
    if (expression(make_dummy_token(), &expr) == RESULT_FAILED) {
        //errors add inside expression
        return RESULT_FAILED;
    }
    
    *node = make_expr_stmt(expr);
    return RESULT_SUCCESS;
}

static void add_error(Token token, const char* message) {
    if (parser.error_count == 256) return;
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
    parser.next = next_token();
    parser.next_next = next_token();
}

static int partition(ParseError* errors, int lo, int hi) {
    ParseError pivot = errors[hi];
    int i = lo - 1;

    for (int j = lo; j <= hi; j++) {
        if (errors[j].token.line <= pivot.token.line) {
            i++;
            ParseError orig_i = errors[i];
            errors[i] = errors[j];
            errors[j] = orig_i;
        }
    }
    return i;
}

static void quick_sort(ParseError* errors, int lo, int hi) {
    int count = hi - lo + 1;
    if (count <= 0) return;

    //making pivot last element is potentially slow, but it's easier to implement
    int p = partition(errors, lo, hi);

    quick_sort(errors, lo, p - 1);
    quick_sort(errors, p + 1, hi);
}

ResultCode parse(const char* source, struct NodeList* nl) {
    init_parser(source);

    while(parser.current.type != TOKEN_EOF) {
        struct Node* decl;
        if (declaration(&decl) == RESULT_SUCCESS) {
            add_node(nl, decl);
        } else {
            synchronize();
        }
    }

    if (parser.error_count > 0) {
        quick_sort(parser.errors, 0, parser.error_count - 1);
        for (int i = 0; i < parser.error_count; i++) {
            printf("[line %d] %s\n", parser.errors[i].token.line, parser.errors[i].message);
        }
        if (parser.error_count == 256) {
            printf("Parsing error count exceeded maximum of 256.\n");
        }
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}

