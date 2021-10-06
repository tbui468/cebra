#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "memory.h"

#define PARSE(fun, var_name, node_ptr, token, msg, ...) \
    if (fun(var_name, node_ptr) == RESULT_FAILED) { \
        if (token.type != TOKEN_DUMMY) { \
            ERROR(token, msg, __VA_ARGS__); \
        } \
        return RESULT_FAILED; \
    }

#define PARSE_WITHOUT_MSG(fun, var_name, node_ptr) \
    PARSE(fun, var_name, node_ptr, make_dummy_token(), "")

#define PARSE_TYPE(var_name, type_ptr, token, msg, ...) \
    if (parse_type(var_name, type_ptr) == RESULT_FAILED) { \
        ERROR(token, msg, __VA_ARGS__); \
    }

#define CONSUME(token_type, token, msg, ...) \
    if (!match(token_type)) { \
        ERROR(token, msg, __VA_ARGS__); \
    }

#define ERROR(tkn, msg, ...) \
{ \
    if (parser.error_count < 256) { \
        char* buf = ALLOCATE_ARRAY(char); \
        buf = GROW_ARRAY(buf, char, 100, 0); \
        snprintf(buf, 99, msg, __VA_ARGS__); \
        struct Error error; \
        error.message = buf; \
        error.token = tkn; \
        parser.errors[parser.error_count] = error; \
        parser.error_count++; \
    } \
    return RESULT_FAILED; \
}


Parser parser;

static ResultCode expression(Token var_name, struct Node** node);
static ResultCode declaration(struct Node** node);
static ResultCode block(struct Node* prepend, struct Node** node);
static ResultCode parse_type(Token var_name, struct Type** type);
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


static ResultCode parse_function(Token var_name, struct Node** node) {
    struct NodeList* params = (struct NodeList*)make_node_list();

    struct Type* param_type = make_array_type();
    if (!match(TOKEN_RIGHT_PAREN)) {
        do {
            struct Node* var_decl;
            if (param_declaration(&var_decl) == RESULT_FAILED) return RESULT_FAILED;

            if (match(TOKEN_EQUAL)) {
                Token param_token = ((DeclVar*)var_decl)->name;
                ERROR(param_token, "Trying to assign parameter '%.*s'.  Function parameters cannot be astypened.", 
                        param_token.length, param_token.start);
            }

            DeclVar* vd = (DeclVar*)var_decl;
            add_type((struct TypeArray*)param_type, vd->type);
            add_node(params, var_decl);
        } while (match(TOKEN_COMMA));
        CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after parameter list.");
    }

    CONSUME(TOKEN_RIGHT_ARROW, parser.previous, "Expect '->' after parameter list.");

    struct Type* ret_type;
    PARSE_TYPE(var_name, &ret_type, parser.previous, "Expect valid return type after function parameters.");

    CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before function body.");

    Token body_start = parser.previous;
    struct Node* body;
    if (block(NULL, &body) == RESULT_FAILED) {
        ERROR(body_start, "Expect '}' at end of function body starting at line %d.", body_start.line);
    }

    struct Type* fun_type = make_fun_type(param_type, ret_type); 

    *node = make_decl_fun(var_name, params, fun_type, body);
    return RESULT_SUCCESS;
}

static ResultCode primary(Token var_name, struct Node** node) {
    if (match(TOKEN_INT) || match(TOKEN_FLOAT) || 
            match(TOKEN_STRING) || match(TOKEN_TRUE) ||
            match(TOKEN_FALSE)) {
        *node = make_literal(parser.previous);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_LIST)) {
        Token identifier = parser.previous;
        CONSUME(TOKEN_LESS, identifier, "Expect '<' after 'List'.");
        struct Type* template_type;
        PARSE_TYPE(var_name, &template_type, var_name, "List must be initialized with valid type: List<[type]>().");
        CONSUME(TOKEN_GREATER, identifier, "Expect '>' after type.");
        *node = make_get_var(identifier, make_list_type(template_type)); 
        return RESULT_SUCCESS;
    } else if (match(TOKEN_MAP)) {
        Token identifier = parser.previous;
        CONSUME(TOKEN_LESS, identifier, "Expect '<' after 'Map'.");
        struct Type* template_type;
        PARSE_TYPE(var_name, &template_type, var_name, "Map must be initialized with valid type: Map<[type]>().");
        CONSUME(TOKEN_GREATER, identifier, "Expect '>' after type.");
        *node = make_get_var(identifier, make_map_type(template_type)); 
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

        if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON) || peek_two(TOKEN_RIGHT_PAREN, TOKEN_RIGHT_ARROW)) {
            PARSE(parse_function, make_dummy_token(), node, name, "Invalid function declaration.");
            return RESULT_SUCCESS;
        }

        //check for group (expression in parentheses)
        struct Node* expr;
        PARSE(expression, var_name, &expr, name, "Expect valid expression after '('.");

        CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after expression.");
        *node = expr;
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
    PARSE_WITHOUT_MSG(primary, var_name, &left);

    while ((match(TOKEN_DOT) || match(TOKEN_LEFT_PAREN) || match(TOKEN_LEFT_BRACKET))) {
        switch(parser.previous.type) {
            case TOKEN_DOT:
                CONSUME(TOKEN_IDENTIFIER, parser.previous, "Expect identifier after '.'.");
                left = make_get_prop(left, parser.previous);
                break;
            case TOKEN_LEFT_PAREN:
                struct NodeList* args = (struct NodeList*)make_node_list();
                if (!match(TOKEN_RIGHT_PAREN)) {
                    do {
                        struct Node* arg;
                        PARSE(expression, var_name, &arg, parser.previous, "Function call argument must be a valid expression.");
                        add_node(args, arg); 
                    } while (match(TOKEN_COMMA));
                    CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after arguments.");
                }

                left = make_call(parser.previous, left, args);
                break;
            case TOKEN_LEFT_BRACKET:
                Token left_bracket = parser.previous;
                struct Node* idx;
                PARSE(expression, var_name, &idx, left_bracket, "Expect string after '[' for Map access, or int for List access.");
                left = make_get_element(parser.previous, left, idx);
                CONSUME(TOKEN_RIGHT_BRACKET, left_bracket, "Expect ']' after index.");
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
        PARSE(unary, var_name, &right, op, "Right side of '%.*s' must be a valid expression.", op.length, op.start);
        *node = make_unary(parser.previous, right);
        return RESULT_SUCCESS;
    }

    return call_dot(var_name, node);
}

static ResultCode cast(Token var_name, struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(unary, var_name, &left);

    while (match(TOKEN_AS)) {
        Token name = parser.previous;
        struct Type* type;
        PARSE_TYPE(var_name, &type, var_name, "Expect type to cast to after 'as'.");
        left = make_cast(name, left, type);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode factor(Token var_name, struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(cast, var_name, &left);

    while (match(TOKEN_STAR) || match(TOKEN_SLASH) || match(TOKEN_MOD)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(cast, var_name, &right, name, "Right hand side of '%.*s' must be valid expresion.", name.length, name.start);
        left = make_binary(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode term(Token var_name, struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(factor, var_name, &left);

    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(factor, var_name, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        left = make_binary(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode relation(Token var_name, struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(term, var_name, &left);

    while (match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL) ||
            match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(term, var_name, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode equality(Token var_name, struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(relation, var_name, &left);

    while (match(TOKEN_EQUAL_EQUAL) || match(TOKEN_BANG_EQUAL) || match(TOKEN_IN)) {
        Token name = parser.previous;
        struct Node* right;
        if (name.type == TOKEN_IN) {
            PARSE(relation, var_name, &right, name, "Right hand side of '%.*s' must be a List.", name.length, name.start);
        } else {
            PARSE(relation, var_name, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        }
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode and(Token var_name, struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(equality, var_name, &left);

    while (match(TOKEN_AND)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(equality, var_name, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode or(Token var_name, struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(and, var_name, &left);

    while (match(TOKEN_OR)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(and, var_name, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode assignment(Token var_name, struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(or, var_name, &left);

    while (match(TOKEN_EQUAL)) {
        Token  name = parser.previous;
        struct Node* right;
        PARSE(expression, var_name, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        if (left->type == NODE_GET_ELEMENT) {
            left = make_set_element(left, right);
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

static ResultCode parse_type(Token var_name, struct Type** type) {
    if (parser.previous.type == TOKEN_COLON && peek_one(TOKEN_EQUAL)) {
        *type = make_decl_type(); //inferred type typenature TODO: should change name to TypeInferred
        return RESULT_SUCCESS;
    }

    //for explicit function type declaration
    if (match(TOKEN_LEFT_PAREN)) {
        struct Type* params = make_array_type();
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                struct Type* param_type;
                PARSE_TYPE(var_name, &param_type, var_name, "Invalid parameter type for function declaration '%.*s'.", var_name.length, var_name.start);
                add_type((struct TypeArray*)params, param_type);
            } while(match(TOKEN_COMMA));
            CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after parameter types.");
        }
        CONSUME(TOKEN_RIGHT_ARROW, parser.previous, "Expect '->' followed by return type.");
        struct Type* ret;
        PARSE_TYPE(var_name, &ret, var_name, "Invalid return type for function declaration '%.*s'.", var_name.length, var_name.start);
        *type = make_fun_type(params, ret);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_INT_TYPE)) {
        *type = make_int_type();
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_FLOAT_TYPE)) {
        *type = make_float_type();
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_BOOL_TYPE)) {
        *type = make_bool_type();
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_STRING_TYPE)) {
        *type = make_string_type();
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_CLASS)) {
        if (match(TOKEN_LESS)) {
            CONSUME(TOKEN_IDENTIFIER, parser.previous, "Expect superclass identifier after '<'.");
            *type = make_class_type(var_name, make_identifier_type(parser.previous));
            return RESULT_SUCCESS;
        }

        *type = make_class_type(var_name, NULL);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_ENUM)) {
        *type = make_enum_type(var_name);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_LIST)) {
        CONSUME(TOKEN_LESS, parser.previous, "Expect '<' after 'List'.");
        struct Type* template_type;
        PARSE_TYPE(var_name, &template_type, var_name, "List '%.*s' declaration type invalid. Specify type inside '<>'.",  var_name.length, var_name.start);
        CONSUME(TOKEN_GREATER, parser.previous, "Expect '>' after type.");
        *type = make_list_type(template_type);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_MAP)) {
        CONSUME(TOKEN_LESS, parser.previous, "Expect '<' after 'Map'.");
        struct Type* template_type;
        PARSE_TYPE(var_name, &template_type, var_name, "Map '%.*s' declaration type invalid. Specify value type inside '<>'.", var_name.length, var_name.start);
        CONSUME(TOKEN_GREATER, parser.previous, "Expect '>' after type.");
        *type = make_map_type(template_type);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_IDENTIFIER)) {
        *type = make_identifier_type(parser.previous);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_NIL) || 
            //function definition
            (parser.previous.type == TOKEN_RIGHT_ARROW && peek_one(TOKEN_LEFT_BRACE)) ||
            //function declaration
            (parser.previous.type == TOKEN_RIGHT_ARROW && peek_one(TOKEN_EQUAL))) {
        *type = make_nil_type();
        return RESULT_SUCCESS;
    }

    return RESULT_FAILED;
}

static ResultCode param_declaration(struct Node** node) {
    CONSUME(TOKEN_IDENTIFIER, parser.previous, "Parameter declaration must begin with an identifier.");
    Token var_name = parser.previous;
    CONSUME(TOKEN_COLON, var_name, "Parameter identifier '%.*s' must be followed by ':'.", var_name.length, var_name.start);

    struct Type* type;
    PARSE_TYPE(var_name, &type, var_name, "Parameter identifier '%.*s' type invalid.", var_name.length, var_name.start);

    *node = make_decl_var(var_name, type, NULL);
    return RESULT_SUCCESS;
}

static ResultCode var_declaration(struct Node** node) {
    match(TOKEN_IDENTIFIER);
    Token var_name = parser.previous;
    match(TOKEN_COLON);  //var_declaration only called if this is true, so no need to check

    struct Type* type;
    PARSE_TYPE(var_name, &type, var_name, "Variable '%.*s' type not declared.  Alternatively, use '%.*s := <expression>' to infer type.", 
            var_name.length, var_name.start, var_name.length, var_name.start);

    CONSUME(TOKEN_EQUAL, var_name, "Variables must be defined at declaration.");

    struct Node* right;

    PARSE(expression, var_name, &right, var_name, "Variable '%.*s' must be assigned to valid expression at declaration.", var_name.length, var_name.start);
    *node = make_decl_var(var_name, type, right);
    return RESULT_SUCCESS;
}

static ResultCode declaration(struct Node** node) {
    if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON)) {
        return var_declaration(node);
    } else if (peek_three(TOKEN_IDENTIFIER, TOKEN_COLON_COLON, TOKEN_ENUM)) {
        match(TOKEN_IDENTIFIER);
        Token enum_name = parser.previous;
        match(TOKEN_COLON_COLON);
        match(TOKEN_ENUM);
        CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before enum body.");

        //create enum type to fill
        struct TypeEnum* type = (struct TypeEnum*)make_enum_type(enum_name);
        struct ObjString* enum_string = make_string(enum_name.start, enum_name.length);
        push_root(to_string(enum_string));

        //check for global name collision
        Value v;
        if (get_from_table(parser.globals, enum_string, &v)) {
            pop_root();
            ERROR(enum_name, "The identifier for this enum is already used.");
        }

        //set globals in parser (which compiler uses)
        set_table(parser.globals, enum_string, to_type((struct Type*)type));
        pop_root();

        //fill in enum type props
        struct NodeList* nl = (struct NodeList*)make_node_list();
        while (!match(TOKEN_RIGHT_BRACE)) {
            CONSUME(TOKEN_IDENTIFIER, parser.previous, "Expect enum list inside enum body.");
            struct Node* dv = make_decl_var(parser.previous, make_int_type(), NULL);
            Token dv_name = ((DeclVar*)dv)->name;
            struct ObjString* prop_name = make_string(dv_name.start, dv_name.length);
            push_root(to_string(prop_name));

            Value v;
            if (get_from_table(&type->props, prop_name, &v)) {
                pop_root();
                ERROR(dv_name, "Element name already used once in this enum.");
            }

            set_table(&type->props, prop_name, to_type(make_int_type()));
            pop_root();
            add_node(nl, dv);
        }

        *node = make_decl_enum(enum_name, nl);

        return RESULT_SUCCESS;
    } else if (peek_three(TOKEN_IDENTIFIER, TOKEN_COLON_COLON, TOKEN_CLASS)) {
        match(TOKEN_IDENTIFIER);
        Token struct_name = parser.previous;
        match(TOKEN_COLON_COLON);

        struct Type* struct_type;
        PARSE_TYPE(struct_name, &struct_type, struct_name, "Invalid type.");
        struct ObjString* struct_string = make_string(struct_name.start, struct_name.length);
        push_root(to_string(struct_string));

        //check for global name collision
        Value v;
        if (get_from_table(parser.globals, struct_string, &v)) {
            pop_root();
            ERROR(struct_name, "The identifier for this struct is already used.");
        }

        //set globals in parser (which compiler uses)
        set_table(parser.globals, struct_string, to_type((struct Type*)struct_type));
        pop_root();

        struct Node* super = parser.previous.type == TOKEN_IDENTIFIER ? 
            make_get_var(parser.previous, NULL) : 
            NULL;

        CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before class body.");

        struct NodeList* nl = (struct NodeList*)make_node_list();
        while (!match(TOKEN_RIGHT_BRACE)) {
            struct Node* decl;
            if (var_declaration(&decl) == RESULT_FAILED) {
                ERROR(parser.previous, "Invalid field declaration in struct '%.*s'.", struct_name.length, struct_name.start);
            }

            DeclVar* dv = (DeclVar*)decl;
            Token prop_name = dv->name;
            struct ObjString* prop_string = make_string(prop_name.start, prop_name.length); 
            push_root(to_string(prop_string));

            struct TypeClass* tc = (struct TypeClass*)struct_type;

            Value v;
            if (get_from_table(&tc->props, prop_string, &v)) {
                pop_root();
                ERROR(prop_name, "Field name already used once in this struct.");
            }

            set_table(&tc->props, prop_string, to_type(dv->type));
            pop_root();

            add_node(nl, decl);
        }

        *node = make_decl_class(struct_name, super, nl);
        return RESULT_SUCCESS;
        /*
        //TODO: PARSE_TYPE should really be the result we used for make_decl_var
        //  instead of making a struct Node* super... (why did we do this?)
        //  and it's using a make_get_var() ?  Why not make_identifier() instead?
        struct Node* super = parser.previous.type == TOKEN_IDENTIFIER ? 
            make_get_var(parser.previous, NULL) : 
            NULL;

        CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before class body.");

        struct NodeList* nl = (struct NodeList*)make_node_list();
        while (!match(TOKEN_RIGHT_BRACE)) {
            struct Node* decl;
            if (var_declaration(&decl) == RESULT_FAILED) {
                ERROR(parser.previous, "Invalid field declaration in struct '%.*s'.", struct_name.length, struct_name.start);
            }

            //TODO: need to get decl and add to struct type
            //  need struct properties for type checking

            add_node(nl, decl);
        }

        //TODO: need to add final struct type to parser.globals

        *node = make_decl_class(struct_name, super, nl);
        return RESULT_SUCCESS;*/
    } else if (peek_three(TOKEN_IDENTIFIER, TOKEN_COLON_COLON, TOKEN_LEFT_PAREN)) {
        match(TOKEN_IDENTIFIER);
        Token var_name = parser.previous;
        match(TOKEN_COLON_COLON);
        match(TOKEN_LEFT_PAREN);

        PARSE(parse_function, var_name, node, var_name, "Invalid function declaration.");

        //TODO: need to get node function final type and add to parser.globals
        
        return RESULT_SUCCESS;
    } else if (match(TOKEN_LEFT_BRACE)) {
        Token name = parser.previous;
        if (block(NULL, node) == RESULT_FAILED) {
            ERROR(name, "Block starting at line %d not closed with '}'.", name.line);
        }
        return RESULT_SUCCESS;
    } else if (match(TOKEN_IF)) {
        Token name = parser.previous;

        struct Node* condition;
        PARSE(expression, make_dummy_token(), &condition, name, "Expect boolean expression after 'if'.");

        CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' after 'if' condition.");

        Token left_brace_token = parser.previous;
        struct Node* then_block;
        if (block(NULL, &then_block) == RESULT_FAILED) {
            ERROR(left_brace_token, "Expect closing '}' to end 'if' statement body.");
        }

        struct Node* else_block = NULL;
        if (match(TOKEN_ELSE)) {
            Token else_token = parser.previous;
            CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' after 'else'.");

            if (block(NULL, &else_block) == RESULT_FAILED) {
                ERROR(else_token, "Expect closing '}' to end 'else' statement body.");
            }
        }

        *node = make_if_else(name, condition, then_block, else_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_WHILE)) {
        Token name = parser.previous;
        struct Node* condition;
        PARSE(expression, make_dummy_token(), &condition, name, "Expect condition after 'while'.");
        CONSUME(TOKEN_LEFT_BRACE, name, "Expect boolean expression and '{' after 'while'.");

        struct Node* then_block;
        if (block(NULL, &then_block) == RESULT_FAILED) {
            ERROR(name, "Expect close '}' after 'while' body.");
        }
        *node = make_while(name, condition, then_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_FOR)) {
        Token name = parser.previous;
        struct Node* initializer = NULL;
        if (!match(TOKEN_COMMA)) {
            if (declaration(&initializer) == RESULT_FAILED) {
                ERROR(name, "Expect initializer or empty space for first item in 'for' loop.");
            }
            CONSUME(TOKEN_COMMA, name, "Expect ',' after for-loop initializer.");
        }

        struct Node* condition = NULL;
        if (!match(TOKEN_COMMA)) {
            PARSE(expression, make_dummy_token(), &condition, name, "Expect condition or empty space for second item in 'for' loop.");
            CONSUME(TOKEN_COMMA, name, "Expect ',' after for-loop condition.");
        }

        struct Node* update = NULL;
        if (!match(TOKEN_LEFT_BRACE)) {
            struct Node* update_expr;
            PARSE(expression, make_dummy_token(), &update_expr, name, "Expect update or empty space for third item in 'for' loop.");
            update = make_expr_stmt(update_expr);
            CONSUME(TOKEN_LEFT_BRACE, name, "Expect '{' after for-loop update.");
        }

        struct Node* then_block;
        if (block(NULL, &then_block) == RESULT_FAILED) {
            ERROR(name, "Expect closing '}' after for-loop body.");
        }

        *node = make_for(name, initializer, condition, update, then_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_FOR_EACH)) {
        Token name = parser.previous;
        struct Node* element;
        if (param_declaration(&element) == RESULT_FAILED) return RESULT_FAILED;

        CONSUME(TOKEN_IN, name, "Expect 'in' after element declaration.");

        struct Node* list;
        PARSE(expression, make_dummy_token(), &list, name, "Expect List identifier after 'in' in foreach loop.");

        //initializer
        Token idx_token = make_token(TOKEN_IDENTIFIER, -1, "_idx_", 5);
        Token zero_token = make_token(TOKEN_INT, -1, "0", 1);
        struct Node* initializer = make_decl_var(idx_token, make_int_type(), make_literal(zero_token));

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
        CONSUME(TOKEN_LEFT_BRACE, name, "Expect '{' before foreach loop body.");

        struct Node* get_element = make_get_element(make_dummy_token(), list, get_idx);
        ((DeclVar*)element)->right = get_element;
        struct Node* then_block;
        if (block(element, &then_block) == RESULT_FAILED) {
            ERROR(name, "Expect closing '}' after foreach loop body.");
        }

        *node = make_for(name, initializer, condition, update, then_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_RIGHT_ARROW)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(expression, make_dummy_token(), &right, name, "Return value must be and expression, or leave empty for 'nil' return.");
        *node = make_return(name, right);
        return RESULT_SUCCESS;
    }

    struct Node* expr;
    PARSE(expression, make_dummy_token(), &expr, parser.previous, "Invalid expression.");

    *node = make_expr_stmt(expr);
    return RESULT_SUCCESS;
}

static void init_parser(const char* source, struct Table* globals) {
    init_lexer(source);
    parser.current = next_token();
    parser.next = next_token();
    parser.next_next = next_token();
    parser.error_count = 0;
    parser.globals = globals;
}

static void free_parser() {
    for (int i = 0; i < parser.error_count; i++) {
        FREE_ARRAY(parser.errors[i].message, char, 100);
    }
}

static int partition(struct Error* errors, int lo, int hi) {
    struct Error pivot = errors[hi];
    int i = lo - 1;

    for (int j = lo; j <= hi; j++) {
        if (errors[j].token.line <= pivot.token.line) {
            i++;
            struct Error orig_i = errors[i];
            errors[i] = errors[j];
            errors[j] = orig_i;
        }
    }
    return i;
}

static void quick_sort(struct Error* errors, int lo, int hi) {
    int count = hi - lo + 1;
    if (count <= 0) return;

    //making pivot last element is potentially slow, but it's easier to implement
    int p = partition(errors, lo, hi);

    quick_sort(errors, lo, p - 1);
    quick_sort(errors, p + 1, hi);
}

ResultCode parse(const char* source, struct NodeList* nl, struct Table* globals) {
    init_parser(source, globals);

    ResultCode result = RESULT_SUCCESS;

    while(parser.current.type != TOKEN_EOF) {
        struct Node* decl;
        if (declaration(&decl) == RESULT_SUCCESS) {
            add_node(nl, decl);
        } else {
            synchronize();
            result = RESULT_FAILED;
        }
    }

    //resolve identifiers in structs (and later functions)
    for (int i = 0; i < parser.globals->capacity; i++) {
        struct Pair* pair = &parser.globals->pairs[i];
        if (pair->value.type == VAL_TYPE) {
            struct Type* type = pair->value.as.type_type;
            if (type->type == TYPE_CLASS) {
                struct TypeClass* tc = (struct TypeClass*)type;
                for (int j = 0; j < tc->props.capacity; j++) {
                    struct Pair* inner_pair = &tc->props.pairs[j];
                    if (inner_pair->value.type == VAL_TYPE) {
                        struct Type* type = inner_pair->value.as.type_type;
                        if (type->type == TYPE_IDENTIFIER) {
                            struct TypeIdentifier* id = (struct TypeIdentifier*)type;
                            struct ObjString* identifier = make_string(id->identifier.start, id->identifier.length);
                            push_root(to_string(identifier));
                            Value resolved_id;
                            if (get_from_table(parser.globals, identifier, &resolved_id)) {
                                set_table(&tc->props, inner_pair->key, resolved_id);
                                pop_root();
                            } else {
                                pop_root();
                                ERROR(make_dummy_token(), "Identifier not declared.");
                            }
                        }
                    }
                }
            }
        }
    }
    //TODO: resolve identifiers in globals
    //  May need two steps for structs
    //      1. resolve all identifiers into Enum/Struct/Function types
    //            (including types in TypeClass->props)
    //      2. Update TypeClass props table by going up the struct heirarchy
    //          and copying all props that don't already exist
    //          BUT, will need to do some checking here to make sure that overwritten
    //          prop types are the same.  Currently doing this check in compiler but
    //          will need to move it all here if we want all global types to be complete
    //          when we get to the compiler.
//    print_table(parser.globals);

    if (parser.error_count > 0) {
        quick_sort(parser.errors, 0, parser.error_count - 1);
        for (int i = 0; i < parser.error_count; i++) {
            printf("Parse Error: [line %d] ", parser.errors[i].token.line);
            printf("%s\n", parser.errors[i].message);
        }
        if (parser.error_count == 256) {
            printf("Parsing error count exceeded maximum of 256.\n");
        }
        free_parser();
        return RESULT_FAILED;
    }

    return result;
}

