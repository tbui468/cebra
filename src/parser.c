#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "memory.h"

Parser parser;

static struct Node* expression();
static struct Node* declaration();
static void add_error(Token token, const char* message);
static struct Node* block();
static struct Sig* read_sig(Token var_name);
static struct Node* param_declaration();
static struct Node* var_declaration();

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
        if (peek_one(TOKEN_WHILE)) return;
        if (peek_one(TOKEN_IF)) return;
        if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON)) return;
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

static struct NodeList* argument_list(Token var_name) {
    struct NodeList* args = (struct NodeList*)make_node_list();
    if (!match(TOKEN_RIGHT_PAREN)) {
        do {
            add_node(args, expression(var_name)); 
        } while (match(TOKEN_COMMA));
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    }

    return args;
}


static struct Node* primary(Token var_name) {
    if (match(TOKEN_INT) || match(TOKEN_FLOAT) || 
        match(TOKEN_STRING) || match(TOKEN_TRUE) ||
        match(TOKEN_FALSE)) {
        return make_literal(parser.previous);
    } else if (match(TOKEN_LIST)) {
        Token identifier = parser.previous;
        consume(TOKEN_LESS, "Expect '<' after 'List'.");
        struct Sig* template_type = read_sig(var_name);
        consume(TOKEN_GREATER, "Expect '>' after type.");
        return make_get_var(identifier, make_list_sig(template_type)); 
    } else if (match(TOKEN_MAP)) {
        Token identifier = parser.previous;
        consume(TOKEN_LESS, "Expect '<' after 'Map'.");
        struct Sig* template_type = read_sig(var_name);
        consume(TOKEN_GREATER, "Expect '>' after type.");
        return make_get_var(identifier, make_map_sig(template_type)); 
    } else if (match(TOKEN_IDENTIFIER)) {
        return make_get_var(parser.previous, NULL);
    } else if (match(TOKEN_LEFT_PAREN)) {
        Token name = parser.previous;
        if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON) ||
            peek_two(TOKEN_RIGHT_PAREN, TOKEN_RIGHT_ARROW)) {
            struct NodeList* params = (struct NodeList*)make_node_list();
            
            struct Sig* param_sig = make_array_sig();
            if (!match(TOKEN_RIGHT_PAREN)) {
                do {
                    struct Node* var_decl = param_declaration();
                    if (var_decl == NULL) {
                        //errors are added in param_declaration
                        return NULL;
                    }
                    if (match(TOKEN_EQUAL)) {
                        add_error(parser.previous, "Function parameters cannot be defined.");
                        return NULL;
                    }
                    DeclVar* vd = (DeclVar*)var_decl;
                    add_sig((struct SigArray*)param_sig, vd->sig);
                    add_node(params, var_decl);
                } while (match(TOKEN_COMMA));
                if (!match(TOKEN_RIGHT_PAREN)) {
                    add_error(parser.previous, "Expect ')' after parameter list.");
                    return NULL;
                }
            }

            if (!match(TOKEN_RIGHT_ARROW)) {
                add_error(parser.previous, "Expect '->' after parameter list.");
                return NULL;
            }

            struct Sig* ret_sig = read_sig(var_name);

            if (!match(TOKEN_LEFT_BRACE)) {
                add_error(parser.previous, "Expect '{' before function body.");
                return NULL;
            }

            struct Node* body = block();
            if (body == NULL) {
                add_error(parser.previous, "Expect '}' after function body.");
                return NULL;
            }

            struct Sig* fun_sig = make_fun_sig(param_sig, ret_sig); 

            return make_decl_fun(var_name, params, fun_sig, body);
        }

        struct Node* expr = expression(var_name);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    } else if (peek_one(TOKEN_CLASS)) {
        struct Sig* right_sig = read_sig(var_name);

        struct Node* super = parser.previous.type == TOKEN_IDENTIFIER ? 
                             make_get_var(parser.previous, NULL) : 
                             NULL;

        consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");

        struct NodeList* nl = (struct NodeList*)make_node_list();
        while (!match(TOKEN_RIGHT_BRACE)) {
            struct Node* decl = var_declaration();
            if (decl->type != NODE_DECL_VAR) {
                add_error(var_name, "Only primitive, function or class definitions allowed in struct body.");
                return NULL;
            }

            add_node(nl, decl);
        }

        return make_decl_class(var_name, super, nl);
    } else if (match(TOKEN_NIL)) {
        return make_nil(parser.previous);
    }

    //add_error(parser.previous, "Invalid token.");
    return NULL;
}

static struct Node* call_dot(Token var_name) {
    struct Node* left = primary(var_name);

    while ((match(TOKEN_DOT) || match(TOKEN_LEFT_PAREN) || match(TOKEN_LEFT_BRACKET))) {
        if (parser.previous.type == TOKEN_DOT) {
            consume(TOKEN_IDENTIFIER, "Expect identifier after '.'.");
            left = make_get_prop(left, parser.previous);
        } else if (parser.previous.type == TOKEN_LEFT_PAREN) {
            left = make_call(parser.previous, left, argument_list(var_name));
        } else { //TOKEN_LEFT_BRACKET
            left = make_get_idx(parser.previous, left, expression(var_name));
            consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
        }
    }

    return left;
}

static struct Node* unary(Token var_name) {
    if (match(TOKEN_MINUS) || match(TOKEN_BANG)) {
        return make_unary(parser.previous, unary(var_name));
    }

    return call_dot(var_name);
}

static struct Node* factor(Token var_name) {
    struct Node* left = unary(var_name);

    while (match(TOKEN_STAR) || match(TOKEN_SLASH) || match(TOKEN_MOD)) {
        if (left == NULL) {
            switch(parser.previous.type) {
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
            return NULL;
        }
        Token name = parser.previous;
        struct Node* right = unary(var_name);
        if (right == NULL) {
            switch(parser.previous.type) {
                case TOKEN_MOD:
                    add_error(parser.previous, "Right hand side of '%' must evaluate to int.");
                    break;
                case TOKEN_STAR:
                    add_error(parser.previous, "Right hand side of '*' must evaluate to int or float.");
                    break;
                case TOKEN_SLASH:
                    add_error(parser.previous, "Right hand side of '/' must evaluate to int or float.");
                    break;
            }
            return NULL;
        }
        left = make_binary(name, left, right);
    }

    return left;
}

static struct Node* term(Token var_name) {
    struct Node* left = factor(var_name);

    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        if (left == NULL) {
            switch(parser.previous.type) {
                case TOKEN_PLUS:
                    add_error(parser.previous, "Left hand side of '+' must evaluate to int or float.");
                    break;
                case TOKEN_MINUS:
                    add_error(parser.previous, "Left hand side of '-' must evaluate to int or float.");
                    break;
            }
            return NULL;
        }
        Token name = parser.previous;
        struct Node* right = factor(var_name);
        if (right == NULL) {
            switch(parser.previous.type) {
                case TOKEN_PLUS:
                    add_error(parser.previous, "Right hand side of '+' must evaluate to int or float.");
                    break;
                case TOKEN_MINUS:
                    add_error(parser.previous, "Right hand side of '-' must evaluate to int or float.");
                    break;
            }
            return NULL;
        }
        left = make_binary(name, left, right);
    }

    return left;
}

static struct Node* relation(Token var_name) {
    struct Node* left = term(var_name);

    while (match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL) ||
           match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL)) {
        if (left == NULL) {
            switch(parser.previous.type) {
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
            return NULL;
        }
        Token name = parser.previous;
        struct Node* right = term(var_name);
        if (right == NULL) {
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
            return NULL;
        }
        left = make_logical(name, left, right);
    }

    return left;
}

static struct Node* equality(Token var_name) {
    struct Node* left = relation(var_name);

    while (match(TOKEN_EQUAL_EQUAL) || match(TOKEN_BANG_EQUAL) || match(TOKEN_IN)) {
        if (left == NULL) {
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
            return NULL;
        }
        Token name = parser.previous;
        struct Node* right = relation(var_name);
        if (right == NULL) {
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
            return NULL;
        }
        left = make_logical(name, left, right);
    }

    return left;
}

static struct Node* and(Token var_name) {
    struct Node* left = equality(var_name);

    while (match(TOKEN_AND)) {
        if (left == NULL) {
            add_error(parser.previous, "Left hand side of 'and' must evaluate to a boolean.");
            return NULL;
        }
        Token name = parser.previous;
        struct Node* right = equality(var_name);
        if (right == NULL) {
            add_error(parser.previous, "Right hand side of 'and' must evaluate to a boolean.");
            return NULL;
        }
        left = make_logical(name, left, right);
    }

    return left;
}

static struct Node* or(Token var_name) {
    struct Node* left = and(var_name);

    while (match(TOKEN_OR)) {
        if (left == NULL) {
            add_error(parser.previous, "Left hand side of 'or' must evaluate to a boolean.");
            return NULL;
        }
        Token name = parser.previous;
        struct Node* right = and(var_name);
        if (right == NULL) {
            add_error(parser.previous, "Right hand side of 'or' must evaluate to a boolean.");
            return NULL;
        }
        left = make_logical(name, left, right);
    }

    return left;
}

static struct Node* assignment(Token var_name) {
    struct Node* left = or(var_name);

    while (match(TOKEN_EQUAL)) {
        if (left == NULL) {
            add_error(parser.previous, "Left hand side of '=' must be assignable.");
            return NULL;
        }

        struct Node* right = expression(var_name);
        if (right == NULL) {
            add_error(parser.previous, "Right hand side of '=' must be an expression.");
            return NULL;
        }
        if (left->type == NODE_GET_IDX) {
            left = make_set_idx(left, right);
        } else if (left->type == NODE_GET_VAR) {
            left = make_set_var(left, right);
        } else { //NODE_GET_PROP
            left = make_set_prop(left, right);
        }
    }

    return left;
}

static struct Node* expression(Token var_name) {
    return assignment(var_name);
}

static struct Node* block() {
    Token name = parser.previous;
    struct NodeList* body = (struct NodeList*)make_node_list();
    while (!match(TOKEN_RIGHT_BRACE)) {
        if (peek_one(TOKEN_EOF)) {
            return NULL;
        }
        struct Node* decl = declaration();
        if (decl == NULL) {
            synchronize();
        } else {
            add_node(body, decl);
        }
    }
    return make_block(name, body);
}

static struct Sig* read_sig(Token var_name) {
    if (parser.previous.type == TOKEN_COLON && peek_one(TOKEN_EQUAL)) {
        return make_decl_sig();
    }

    if (match(TOKEN_LEFT_PAREN)) {
        struct Sig* params = make_array_sig();
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                add_sig((struct SigArray*)params, read_sig(var_name));
            } while(match(TOKEN_COMMA));
            consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameter types.");
        }
        consume(TOKEN_RIGHT_ARROW, "Expect '->' followed by return type.");
        struct Sig* ret = read_sig(var_name);
        return make_fun_sig(params, ret);
    }

    if (match(TOKEN_INT_TYPE)) 
        return make_prim_sig(VAL_INT);

    if (match(TOKEN_FLOAT_TYPE))
        return make_prim_sig(VAL_FLOAT);
    
    if (match(TOKEN_BOOL_TYPE))
        return make_prim_sig(VAL_BOOL);
    
    if (match(TOKEN_STRING_TYPE))
        return make_prim_sig(VAL_STRING);

    if (match(TOKEN_CLASS)) {
        if (match(TOKEN_LESS)) {
            consume(TOKEN_IDENTIFIER, "Expect superclass identifier after '<'.");
            return make_class_sig(var_name);
        }

        return make_class_sig(var_name);
    }

    if (match(TOKEN_LIST)) {
        consume(TOKEN_LESS, "Expect '<' after 'List'.");
        struct Sig* template_type = read_sig(var_name);
        consume(TOKEN_GREATER, "Expect '>' after type."); 
        return make_list_sig(template_type);
    }

    if (match(TOKEN_MAP)) {
        consume(TOKEN_LESS, "Expect '<' after 'Map'.");
        struct Sig* template_type = read_sig(var_name);
        consume(TOKEN_GREATER, "Expect '>' after type."); 
        return make_map_sig(template_type);
    }

    if (match(TOKEN_IDENTIFIER)) {
        return make_identifier_sig(parser.previous);
    }

    return make_prim_sig(VAL_NIL);
}

static struct Node* param_declaration() {
    if (!match(TOKEN_IDENTIFIER)) {
        add_error(parser.previous, "Parameter must begin with identifier.");
        return NULL;
    }
    Token var_name = parser.previous;
    if (!match(TOKEN_COLON)) {
        add_error(var_name, "Parameter identifer must be followed by ':'.");
        return NULL;
    }

    struct Sig* sig = read_sig(var_name);
    if (sig_is_type(sig, VAL_NIL)) {
        add_error(var_name, "Parameter must be a valid type.");
        return NULL;
    }

    return make_decl_var(var_name, sig, make_nil(parser.previous));
}

static struct Node* var_declaration() {
    match(TOKEN_IDENTIFIER);
    Token var_name = parser.previous;
    match(TOKEN_COLON);  //var_declaration only called if this is true, so no need to check

    struct Sig* sig = read_sig(var_name);

    if (!match(TOKEN_EQUAL)) {
        add_error(var_name, "Variables must be defined at declaration.");
        return NULL;
    }
    struct Node* right = expression(var_name);
    if (right == NULL) {
        add_error(var_name, "Variable must be assigned to valid expression.");
        return NULL;
    }
    return make_decl_var(var_name, sig, right);
}

static struct Node* declaration() {
    if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON)) {
        return var_declaration();
    } else if (match(TOKEN_LEFT_BRACE)) {
        Token name = parser.previous;
        struct Node* b = block();
        if (b == NULL) {
            add_error(name, "Expect '}' after block body.");
            return NULL;
        }
        return b;
    } else if (match(TOKEN_IF)) {
        Token name = parser.previous;

        struct Node* condition = expression(make_dummy_token());
        if (condition == NULL) {
            add_error(name, "Expect boolean expression after 'if'.");
            return NULL;
        }

        if (!match(TOKEN_LEFT_BRACE)) {
            add_error(name, "Expect '{' after 'if' condition.");
            return NULL;
        }

        struct Node* then_block = block();
        if (then_block == NULL) {
            add_error(name, "Expect '}' after 'if' statement body.");
            return NULL;
        }

        struct Node* else_block = NULL;
        if (match(TOKEN_ELSE)) {
            if (!match(TOKEN_LEFT_BRACE)) {
                add_error(name, "Expect '{' after 'else'.");
                return NULL;
            }
            else_block = block();
            if (else_block == NULL) {
                add_error(name, "Expect '}' after 'else' statement body.");
                return NULL;
            }
        }

        return make_if_else(name, condition, then_block, else_block);
    } else if (match(TOKEN_WHILE)) {
        Token name = parser.previous;
        struct Node* condition = expression(make_dummy_token());
        if (condition == NULL || !match(TOKEN_LEFT_BRACE)) {
            add_error(name, "Expect boolean expression and '{' after 'while'.");
            return NULL;
        }
        struct Node* then_block = block();
        if (then_block == NULL) {
            add_error(name, "Expect '}' after 'while' body.");
            return NULL;
        }
        return make_while(name, condition, then_block);
    } else if (match(TOKEN_FOR)) {
        Token name = parser.previous;
        struct Node* initializer = NULL;
        if (!match(TOKEN_COMMA)) {
            initializer = declaration();
            if (initializer == NULL) {
                add_error(name, "Expect initializer or empty space for first item in 'for' loop.");
                return NULL;
            }
            if (!match(TOKEN_COMMA)) {
                add_error(name, "Expect ',' after for-loop initializer.");
                return NULL; 
            }
        }

        struct Node* condition = NULL;
        if (!match(TOKEN_COMMA)) {
            condition = expression(make_dummy_token());
            if (condition == NULL) {
                add_error(name, "Expect condition or empty space for second item in 'for' loop.");
                return NULL;
            }
            if (!match(TOKEN_COMMA)) {
                add_error(name, "Expect ',' after for-loop condition.");
                return NULL; 
            }
        }

        struct Node* update = NULL;
        if (!match(TOKEN_LEFT_BRACE)) {
            update = expression(make_dummy_token());
            if (update == NULL) {
                add_error(name, "Expect update or empty space for third item in 'for' loop.");
                return NULL;
            }
            if (!match(TOKEN_LEFT_BRACE)) {
                add_error(name, "Expect '{' after for-loop update.");
                return NULL; 
            }
        }

        struct Node* then_block = block();
        if (then_block == NULL) {
            add_error(name, "Expect '}' after for-loop body.");
            return NULL;
        }

        return make_for(name, initializer, condition, update, then_block);
    } else if (match(TOKEN_RIGHT_ARROW)) {
        Token name = parser.previous;
        struct Node* right = expression(make_dummy_token()); //NULL if '}'
        return make_return(name, right);
    }

    return make_expr_stmt(expression(make_dummy_token()));
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
        struct Node* decl = declaration();
        if (decl == NULL) {
            synchronize();
        } else {
            add_node(nl, decl);
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

