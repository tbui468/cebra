#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "memory.h"

#define PARSE(fun, node_ptr, token, ...) \
    if (fun(node_ptr) == RESULT_FAILED) { \
        if (token.type != TOKEN_DUMMY) { \
            ERROR(token, __VA_ARGS__); \
        } \
        return RESULT_FAILED; \
    }

#define PARSE_WITHOUT_MSG(fun, node_ptr) \
    PARSE(fun, node_ptr, make_dummy_token(), "")

#define PARSE_TYPE(type_ptr, token, ...) \
    if (parse_type(type_ptr) == RESULT_FAILED) { \
        ERROR(token, __VA_ARGS__); \
    }

#define CONSUME(token_type, token, ...) \
    if (!match(token_type)) { \
        ERROR(token, __VA_ARGS__); \
    }

#define ERROR(tkn, ...) \
{ \
    if (parser.error_count < MAX_ERRORS) { \
        char* buf = ALLOCATE_ARRAY(char); \
        buf = GROW_ARRAY(buf, char, 100, 0); \
        snprintf(buf, 99, __VA_ARGS__); \
        struct Error error; \
        error.message = buf; \
        error.token = tkn; \
        parser.errors[parser.error_count] = error; \
        parser.error_count++; \
    } \
    return RESULT_FAILED; \
}

#define PARSE_ERROR_IF(error_condition, tkn, ...) \
            if (error_condition) { \
                if (parser.error_count < MAX_ERRORS) { \
                    char* buf = ALLOCATE_ARRAY(char); \
                    buf = GROW_ARRAY(buf, char, 100, 0); \
                    snprintf(buf, 99, __VA_ARGS__); \
                    struct Error error; \
                    error.message = buf; \
                    error.token = tkn; \
                    parser.errors[parser.error_count] = error; \
                    parser.error_count++; \
                } \
                result = RESULT_FAILED; \
            }


Parser parser;

static ResultCode declaration(struct Node** node);
static ResultCode block(struct Node* prepend, struct Node** node);
static ResultCode parse_type(struct Type** type);
static ResultCode param_declaration(struct Node** node);
static ResultCode assignment(struct Node** node, int expected);
static ResultCode parse_expression(struct Node** node);
static ResultCode resolve_type_identifiers(struct Type** type, struct Table* globals);

static void print_all_tokens() {
    printf("******start**********\n");
    printf("%d\n", parser.previous.type);
    printf("%d\n", parser.current.type);
    printf("%d\n", parser.next.type);
    printf("%d\n", parser.next_next.type);
    printf("******end**********\n");
}

void add_to_compile_pass(struct Node* decl, struct NodeList* first, struct NodeList* second) {
    switch(decl->type) {
        case NODE_ENUM:
        case NODE_STRUCT: //include in first pass
            add_node(first, decl);
            break;
        case NODE_FUN:
            if (((DeclFun*)decl)->anonymous) add_node(second, decl);
            else add_node(first, decl);
            break;
        case NODE_LIST: {
            struct NodeList* nl = (struct NodeList*)decl;
            for (int i = 0; i < nl->count; i++) {
                add_node(second, nl->nodes[i]);
            }
            break;
        }
        default:
            add_node(second, decl);
            break;
    }
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

static ResultCode parse_function_parameters_and_types(struct NodeList* params, struct TypeArray* param_types) {
    //no parameters
    if (match(TOKEN_RIGHT_PAREN)) return RESULT_SUCCESS;

    do {
        struct Node* var_decl;
        if (param_declaration(&var_decl) == RESULT_FAILED) return RESULT_FAILED;

        if (match(TOKEN_EQUAL)) {
            Token param_token = ((DeclVar*)var_decl)->name;
            ERROR(param_token, "Trying to assign parameter '%.*s'.  Function parameters cannot be assigned.", 
                    param_token.length, param_token.start);
        }

        DeclVar* vd = (DeclVar*)var_decl;
        add_type(param_types, vd->type);
        add_node(params, var_decl);
    } while (match(TOKEN_COMMA));
    CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after parameter list.");

    return RESULT_SUCCESS;
}
static ResultCode parse_function_return_types(struct TypeArray* return_types) {
    Token param_token = parser.previous;
    CONSUME(TOKEN_RIGHT_ARROW, parser.previous, "Expect '->' after parameter list.");
    CONSUME(TOKEN_LEFT_PAREN, parser.previous, "Expect '(' before return type list.");
    //no return
    if (match(TOKEN_RIGHT_PAREN)) {
        add_type(return_types, make_nil_type());
        return RESULT_SUCCESS;
    }

    do {
        struct Type* single_ret_type;
        PARSE_TYPE(&single_ret_type, param_token, "Expect valid return type after function parameters.");
        add_type(return_types, single_ret_type);
    } while(match(TOKEN_COMMA));
    CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after return type list.");

    return RESULT_SUCCESS;
}

static ResultCode parse_static_function(Token name, struct Node** node) {
    struct NodeList* params = (struct NodeList*)make_node_list();
    struct TypeArray* param_types = make_type_array();
    if (parse_function_parameters_and_types(params, param_types) == RESULT_FAILED) {
        return RESULT_FAILED;
    }

    struct TypeArray* return_types = make_type_array();
    if (parse_function_return_types(return_types) == RESULT_FAILED) {
        return RESULT_FAILED;
    }

    struct Type* fun_type = make_fun_type(param_types, return_types);

    //create type and add to global types for static functions
    struct ObjString* fun_string = make_string(name.start, name.length);
    push_root(to_string(fun_string));
    Value v;
    if (get_entry(parser.globals, fun_string, &v)) {
        pop_root();
        ERROR(name, "The identifier for this function is already used.");
    }

    //set globals in parser (which compiler uses)
    set_entry(parser.globals, fun_string, to_type(fun_type));
    pop_root();

    CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before function body.");
    Token body_start = parser.previous;
    struct Node* body;
    if (block(NULL, &body) == RESULT_FAILED) {
        ERROR(body_start, "Expect '}' at end of function body starting at line %d.", body_start.line);
    }

    //fun_type is only necessary for anonymous functions 
    bool anonymous = false;
    *node = make_decl_fun(name, params, fun_type, body, anonymous);
    return RESULT_SUCCESS;
}

static ResultCode parse_anonymous_function(struct Node** node) {
    struct NodeList* params = (struct NodeList*)make_node_list();
    struct TypeArray* param_types = make_type_array();
    if (parse_function_parameters_and_types(params, param_types) == RESULT_FAILED) {
        return RESULT_FAILED;
    }

    struct TypeArray* return_types = make_type_array();
    if (parse_function_return_types(return_types) == RESULT_FAILED) {
        return RESULT_FAILED;
    }

    struct Type* fun_type = make_fun_type(param_types, return_types);

    CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before function body.");
    Token body_start = parser.previous;
    struct Node* body;
    if (block(NULL, &body) == RESULT_FAILED) {
        ERROR(body_start, "Expect '}' at end of function body starting at line %d.", body_start.line);
    }

    //fun_type is only necessary for anonymous functions 
    bool anonymous = true;
    *node = make_decl_fun(make_dummy_token(), params, fun_type, body, anonymous);
    return RESULT_SUCCESS;
}

static ResultCode primary(struct Node** node) {
    if (match(TOKEN_INT) || match(TOKEN_FLOAT) || 
            match(TOKEN_STRING) || match(TOKEN_TRUE) ||
            match(TOKEN_FALSE)) {
        *node = make_literal(parser.previous);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_LIST)) {
        Token identifier = parser.previous;
        CONSUME(TOKEN_LESS, identifier, "Expect '<' after 'List'.");
        struct Type* template_type;
        PARSE_TYPE(&template_type, identifier, "List must be initialized with valid type: List<[type]>().");
        CONSUME(TOKEN_GREATER, identifier, "Expect '>' after type.");
        CONSUME(TOKEN_LEFT_PAREN, identifier, "Create container using '()'.");
        CONSUME(TOKEN_RIGHT_PAREN, identifier, "Create container using '()'.");
        *node = make_decl_container(identifier, make_list_type(template_type)); 
        return RESULT_SUCCESS;
    } else if (match(TOKEN_MAP)) {
        Token identifier = parser.previous;
        CONSUME(TOKEN_LESS, identifier, "Expect '<' after 'Map'.");
        struct Type* template_type;
        PARSE_TYPE(&template_type, identifier, "Map must be initialized with valid type: Map<[type]>().");
        CONSUME(TOKEN_GREATER, identifier, "Expect '>' after type.");
        CONSUME(TOKEN_LEFT_PAREN, identifier, "Create container using '()'.");
        CONSUME(TOKEN_RIGHT_PAREN, identifier, "Create container using '()'.");
        *node = make_decl_container(identifier, make_map_type(template_type)); 
        return RESULT_SUCCESS;
    } else if (match(TOKEN_IDENTIFIER)) {
        Token id = parser.previous;
        //checking for TOKEN_IDENTIFIER + TOKEN_COLON before getting to this function
        *node = make_get_var(id);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_STRING_TYPE)) {
        Token token = parser.previous;
        token.type = TOKEN_IDENTIFIER;
        *node = make_get_var(token);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_LEFT_PAREN)) {
        Token name = parser.previous;

        if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON) || peek_two(TOKEN_RIGHT_PAREN, TOKEN_RIGHT_ARROW)) {
            if (parse_anonymous_function(node) == RESULT_FAILED) { 
                ERROR(name, "Invalid anonymous function declaration."); 
            }
            return RESULT_SUCCESS;
        }

        //check for group (expression in parentheses)
        struct Node* expr;
        PARSE(parse_expression, &expr, name, "Expect valid expression after '('.");

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

static ResultCode call_dot(struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(primary, &left);

    while ((match(TOKEN_DOT) || match(TOKEN_LEFT_PAREN) || match(TOKEN_LEFT_BRACKET))) {
        switch(parser.previous.type) {
            case TOKEN_DOT: {
                CONSUME(TOKEN_IDENTIFIER, parser.previous, "Expect identifier after '.'.");
                left = make_get_prop(left, parser.previous);
                break;
            }
            case TOKEN_LEFT_PAREN: {
                struct NodeList* args = (struct NodeList*)make_node_list();
                if (!match(TOKEN_RIGHT_PAREN)) {
                    do {
                        struct Node* arg;
                        PARSE(parse_expression, &arg, parser.previous, "Function call argument must be a valid expression.");
                        add_node(args, arg); 
                    } while (match(TOKEN_COMMA));
                    CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after arguments.");
                }

                left = make_call(parser.previous, left, args);
                break;
            }
            case TOKEN_LEFT_BRACKET: {
                Token left_bracket = parser.previous;
                struct Node* idx;
                PARSE(parse_expression, &idx, left_bracket, "Expect string after '[' for Map access, or int for List access.");
                if (match(TOKEN_COLON)) {
                    struct Node* end_idx;
                    PARSE(parse_expression, &end_idx, left_bracket, "Expect end index for array slicing.");
                    left = make_slice_string(left_bracket, left, idx, end_idx);
                } else {
                    left = make_get_element(parser.previous, left, idx);
                }
                //left = make_get_element(parser.previous, left, idx); //TODO: delete this when addin string slicing
                CONSUME(TOKEN_RIGHT_BRACKET, left_bracket, "Expect ']' after index.");
                break;
            }
            default:
                break;
        }
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode unary(struct Node** node) {
    if (match(TOKEN_MINUS) || match(TOKEN_BANG)) {
        Token op = parser.previous;
        struct Node* right;
        PARSE(unary, &right, op, "Right side of '%.*s' must be a valid expression.", op.length, op.start);
        *node = make_unary(parser.previous, right);
        return RESULT_SUCCESS;
    }

    return call_dot(node);
}

static ResultCode cast(struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(unary, &left);

    while (match(TOKEN_AS)) {
        Token name = parser.previous;
        struct Type* type;
        PARSE_TYPE(&type, name, "Expect type to cast to after 'as'.");
        left = make_cast(name, left, type);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode factor(struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(cast, &left);

    while (match(TOKEN_STAR) || match(TOKEN_SLASH) || match(TOKEN_MOD)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(cast, &right, name, "Right hand side of '%.*s' must be valid expresion.", name.length, name.start);
        left = make_binary(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode term(struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(factor, &left);

    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(factor, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        left = make_binary(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode relation(struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(term, &left);

    while (match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL) ||
            match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(term, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode equality(struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(relation, &left);

    while (match(TOKEN_EQUAL_EQUAL) || match(TOKEN_BANG_EQUAL) || match(TOKEN_IN)) {
        Token name = parser.previous;
        struct Node* right;
        if (name.type == TOKEN_IN) {
            PARSE(relation, &right, name, "Right hand side of '%.*s' must be a List.", name.length, name.start);
        } else {
            PARSE(relation, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        }
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode and(struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(equality, &left);

    while (match(TOKEN_AND)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(equality, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}

static ResultCode or(struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(and, &left);

    while (match(TOKEN_OR)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(and, &right, name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
        left = make_logical(name, left, right);
    }

    *node = left;
    return RESULT_SUCCESS;
}


static ResultCode parse_sequence(struct Node** node) {

    struct NodeList* left = (struct NodeList*)make_node_list();
    Token name = make_dummy_token();
    do {
        //check for declarations with explicit types (to avoid problems with string slicing syntax and colons)
        if (peek_two(TOKEN_IDENTIFIER, TOKEN_COLON)) {
            match(TOKEN_IDENTIFIER);
            name = parser.previous;
            match(TOKEN_COLON);
            struct Type* type;
            PARSE_TYPE(&type, name, "Expected valid type after ':'.");
            add_node(left, make_decl_var(name, type, NULL));
        } else {
            struct Node* expr;
            PARSE(or, &expr, parser.previous, "Invalid expression.");
            name = parser.previous;
            add_node(left, expr);
        }
    } while (match(TOKEN_COMMA));


    if (match(TOKEN_EQUAL) || match(TOKEN_COLON_EQUAL)) {
        name = parser.previous;
        struct Node* right;
        if (parse_sequence(&right) == RESULT_FAILED) {
            return RESULT_FAILED;
        }
        *node = make_sequence(name, left, right);
    } else {
        *node = make_sequence(name, left, NULL);
    }
    return RESULT_SUCCESS;
}

static ResultCode parse_expression(struct Node** node) {
    struct Node* left;
    PARSE_WITHOUT_MSG(or, &left);

    while (match(TOKEN_EQUAL) || match(TOKEN_COLON_EQUAL)) { 
        Token name = parser.previous;
        struct Node* right;
        PARSE(parse_expression, &right, name, "Invalid expression.");

        if (name.type == TOKEN_EQUAL) {
            if (left->type == NODE_GET_ELEMENT) {
                left = make_set_element(left, right);
            } else if (left->type == NODE_GET_VAR) {
                left = make_set_var(left, right);
            } else if (left->type == NODE_GET_PROP) {
                left = make_set_prop(left, right);
            } else if (left->type == NODE_DECL_VAR) {
                ((DeclVar*)left)->right = right;
            }
        } else { //TOKEN_COLON_EQUAL
            if (left->type == NODE_GET_VAR) {
                GetVar* gv = (GetVar*)left;
                left = make_decl_var(gv->name, make_infer_type(), right);
            } else if (left->type == NODE_DECL_VAR) {
                ERROR(parser.previous, "Type inference using ':=' cannot be used if type is already explicitly declared.");
            }
        }
    }


    *node = left;
    return RESULT_SUCCESS;
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
        if (peek_one(TOKEN_IMPORT)) {
            ERROR(parser.previous, "'import' can only be used in top script level.");
            return RESULT_FAILED;
        }
        struct Node* decl;
        if (declaration(&decl) == RESULT_SUCCESS) {
            add_to_compile_pass(decl, parser.statics_nl, body);
        } else {
            synchronize();
        }
    }
    *node = make_block(name, body);
    return RESULT_SUCCESS;
}

//Note: TOKEN_COLON is consume before if a variable declaration
static ResultCode parse_type(struct Type** type) {
    if (match(TOKEN_LEFT_PAREN)) {
        struct TypeArray* params = make_type_array();
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                struct Type* param_type;
                PARSE_TYPE(&param_type, parser.previous, "Invalid parameter type for function declaration.");
                add_type(params, param_type);
            } while(match(TOKEN_COMMA));
            CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after parameter types.");
        }
        CONSUME(TOKEN_RIGHT_ARROW, parser.previous, "Expect '->' followed by return type.");
        CONSUME(TOKEN_LEFT_PAREN, parser.previous, "Expect '(' before return types.");
        struct TypeArray* returns = make_type_array();
        if (match(TOKEN_RIGHT_PAREN)) {
            add_type(returns, make_nil_type());
        } else {
            do {
                struct Type* ret_type;
                PARSE_TYPE(&ret_type, parser.previous, "Invalid return type for function declaration.");
                add_type(returns, ret_type);
            } while(match(TOKEN_COMMA));
            CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after return types.");
        }
        *type = make_fun_type(params, returns);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_FILE_TYPE)) {
        *type = make_file_type();
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

    if (match(TOKEN_LIST)) {
        CONSUME(TOKEN_LESS, parser.previous, "Expect '<' after 'List'.");
        struct Type* template_type;
        PARSE_TYPE(&template_type, parser.previous, "List declaration type invalid. Specify type inside '<>'.");
        CONSUME(TOKEN_GREATER, parser.previous, "Expect '>' after type.");
        *type = make_list_type(template_type);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_MAP)) {
        CONSUME(TOKEN_LESS, parser.previous, "Expect '<' after 'Map'.");
        struct Type* template_type;
        PARSE_TYPE(&template_type, parser.previous, "Map declaration type invalid. Specify value type inside '<>'.");
        CONSUME(TOKEN_GREATER, parser.previous, "Expect '>' after type.");
        *type = make_map_type(template_type);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_IDENTIFIER)) {
        *type = make_identifier_type(parser.previous);
        return RESULT_SUCCESS;
    }

    if (match(TOKEN_NIL)) {
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
    PARSE_TYPE(&type, var_name, "Parameter identifier '%.*s' type invalid.", var_name.length, var_name.start);

    *node = make_decl_var(var_name, type, NULL);
    return RESULT_SUCCESS;
}

static ResultCode add_prop_to_struct_type(struct TypeStruct* tc, DeclVar* dv) {
    Token prop_name = dv->name;
    struct ObjString* prop_string = make_string(prop_name.start, prop_name.length); 
    push_root(to_string(prop_string));

    Value v;
    if (get_entry(&tc->props, prop_string, &v)) {
        pop_root();
        ERROR(prop_name, "Field name already used once in this struct.");
    }

    set_entry(&tc->props, prop_string, to_type(dv->type));
    pop_root();
    return RESULT_SUCCESS;
}

static ResultCode parse_import() {
    match(TOKEN_IMPORT);
    CONSUME(TOKEN_IDENTIFIER, parser.previous, "Expected module name after 'import'.");
    parser.imports[parser.import_count++] = parser.previous;
    return RESULT_SUCCESS;
}

static ResultCode parse_var_declaration(struct Node** node) {
    match(TOKEN_IDENTIFIER);
    Token name = parser.previous;
    struct Type* type = make_infer_type();
    if (match(TOKEN_COLON)) {
        PARSE_TYPE(&type, name, "Invalid type for variable declaration.");
        match(TOKEN_EQUAL); 
    } else {
        match(TOKEN_COLON_EQUAL);
    }
    struct Node* right;
    PARSE(parse_expression, &right, name, "Expected valid right side expression for assignment.");
    *node = make_decl_var(name, type, right);
    return RESULT_SUCCESS;
}

static ResultCode declaration(struct Node** node) {
    if (peek_three(TOKEN_IDENTIFIER, TOKEN_COLON_COLON, TOKEN_ENUM)) {
        match(TOKEN_IDENTIFIER);
        Token enum_name = parser.previous;
        match(TOKEN_COLON_COLON);
        match(TOKEN_ENUM);
        CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before enum body.");

        //create enum type to fill with props
        struct TypeEnum* type = (struct TypeEnum*)make_enum_type(enum_name);
        struct ObjString* enum_string = make_string(enum_name.start, enum_name.length);
        push_root(to_string(enum_string));

        //check for global name collision
        Value v;
        if (get_entry(parser.globals, enum_string, &v)) {
            pop_root();
            ERROR(enum_name, "The identifier for this enum is already used.");
        }

        //set globals in parser (which compiler uses for type checking)
        set_entry(parser.globals, enum_string, to_type((struct Type*)type));
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
            if (get_entry(&type->props, prop_name, &v)) {
                pop_root();
                ERROR(dv_name, "Element name already used once in this enum.");
            }

            set_entry(&type->props, prop_name, to_type(make_int_type()));
            pop_root();
            add_node(nl, dv);
        }

        *node = make_decl_enum(enum_name, nl);

        return RESULT_SUCCESS;
    } else if (peek_three(TOKEN_IDENTIFIER, TOKEN_COLON_COLON, TOKEN_STRUCT)) {
        match(TOKEN_IDENTIFIER);
        Token struct_name = parser.previous;
        match(TOKEN_COLON_COLON);

        //not calling parse_type() to avoid passing Token with every call of parse_type
        struct Type* struct_type;
        match(TOKEN_STRUCT);
        if (match(TOKEN_LESS)) {
            CONSUME(TOKEN_IDENTIFIER, parser.previous, "Expect superclass identifier after '<'.");
            Token super_identifier = parser.previous;
            struct_type = make_struct_type(struct_name, make_identifier_type(super_identifier));
        } else {
            struct_type = make_struct_type(struct_name, NULL);
        }

        struct ObjString* struct_string = make_string(struct_name.start, struct_name.length);
        push_root(to_string(struct_string));

        //check for global name collision
        Value v;
        if (get_entry(parser.globals, struct_string, &v)) {
            pop_root();
            ERROR(struct_name, "The identifier for this struct is already used.");
        }

        //set globals in parser (which compiler uses)
        set_entry(parser.globals, struct_string, to_type(struct_type));
        pop_root();

        struct Node* super = parser.previous.type == TOKEN_IDENTIFIER ? 
                             make_get_var(parser.previous) : 
                             NULL;

        CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before class body.");

        struct NodeList* nl = (struct NodeList*)make_node_list();
        struct TypeStruct* tc = (struct TypeStruct*)struct_type;
        while (!match(TOKEN_RIGHT_BRACE)) {
            struct Node* dv;
            if (parse_var_declaration(&dv) == RESULT_FAILED) return RESULT_FAILED;
            if (add_prop_to_struct_type(tc, (DeclVar*)dv) == RESULT_FAILED) return RESULT_FAILED;
            add_node(nl, dv);
        }

        *node = make_decl_struct(struct_name, super, nl);
        return RESULT_SUCCESS;
    } else if (peek_three(TOKEN_IDENTIFIER, TOKEN_COLON_COLON, TOKEN_LEFT_PAREN)) {
        match(TOKEN_IDENTIFIER);
        Token fun_name = parser.previous;
        match(TOKEN_COLON_COLON);
        match(TOKEN_LEFT_PAREN);

        if (parse_static_function(fun_name, node) == RESULT_FAILED) {
            return RESULT_FAILED;
        }
        
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
        PARSE(parse_expression, &condition, name, "Expect single boolean expression after 'if'.");

        struct Node* then_block;
        if (declaration(&then_block) == RESULT_FAILED) {
            ERROR(name, "Expected a body statement after for 'if' statement.");
            return RESULT_FAILED;
        }

        struct Node* else_block = NULL;
        if (match(TOKEN_ELSE)) {
            Token else_token = parser.previous;
            if (declaration(&else_block) == RESULT_FAILED) {
                ERROR(else_token, "Expected a body statement after for 'else' statement.");
                return RESULT_FAILED;
            }
        }

        *node = make_if_else(name, condition, then_block, else_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_WHEN)) {
        Token name = parser.previous;
        struct Node* left;
        PARSE(parse_expression, &left, name, "Expected a valid expression after 'when'.");
        CONSUME(TOKEN_LEFT_BRACE, name, "Expect '{' after 'when' and variable.");
        struct NodeList* cases = (struct NodeList*)make_node_list();

        while (match(TOKEN_IS)) {
            Token is = parser.previous;

            struct Node* right;
            PARSE(parse_expression, &right, is, "Expected expression to test equality with variable after 'when'.");
            Token make_token(TokenType type, int line, const char* start, int length);
            Token equal = make_token(TOKEN_EQUAL_EQUAL, is.line, "==", 2);
            struct Node* condition = make_logical(equal, left, right);

            struct Node* body = NULL;
            if (declaration(&body) == RESULT_FAILED) {
                ERROR(is, "Expect statment body after 'is'.");
            }

            struct Node* if_stmt = make_if_else(is, condition, body, NULL);
            add_node(cases, if_stmt);
        }

        //make_if_else(else token, true, block, NULL)
        if (match(TOKEN_ELSE)) {
            Token else_token = parser.previous;
            Token true_token = make_token(TOKEN_TRUE, else_token.line, "true", 4);
            struct Node* condition = make_literal(true_token);

            struct Node* body = NULL;
            if (declaration(&body) == RESULT_FAILED) {
                ERROR(else_token, "Expect statement body after 'else' in 'when' body.");
            }
            add_node(cases, make_if_else(else_token, condition, body, NULL));
        }
        CONSUME(TOKEN_RIGHT_BRACE, name, "Expected '}' to close 'when' body.");

        *node = make_when(name, cases);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_WHILE)) {
        Token name = parser.previous;
        struct Node* condition;
        PARSE(parse_expression, &condition, name, "Expect condition after 'while'.");

        struct Node* then_block;
        if (declaration(&then_block) == RESULT_FAILED) {
            ERROR(name, "Expected body statement for 'while' loop.");
        }

        *node = make_while(name, condition, then_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_FOR)) {
        Token name = parser.previous;
        struct Node* initializer = NULL;

        if (!match(TOKEN_UNDER_SCORE)) {
            if (parse_var_declaration(&initializer) == RESULT_FAILED) return RESULT_FAILED;
        }
        CONSUME(TOKEN_COMMA, name, "Expect ',' after for-loop initializer.");

        struct Node* condition = NULL;
        PARSE(parse_expression, &condition, name, "Expect update for second item in 'for' loop.");
        CONSUME(TOKEN_COMMA, name, "Expect ',' after for-loop condition.");

        struct Node* update = NULL;
        if (!match(TOKEN_UNDER_SCORE)) {
            struct Node* update_expr;
            PARSE(parse_expression, &update_expr, name, "Expect update or empty space for third item in 'for' loop.");
            update = make_expr_stmt(update_expr);
        }

        struct Node* then_block;
        if (declaration(&then_block) == RESULT_FAILED) {
            ERROR(name, "Expect body statement for 'for' loop.");
        }

        *node = make_for(name, initializer, condition, update, then_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_FOR_EACH)) {
        Token name = parser.previous;
        struct Node* element;
        if (param_declaration(&element) == RESULT_FAILED) return RESULT_FAILED;

        CONSUME(TOKEN_IN, name, "Expect 'in' after element declaration.");

        struct Node* list;
        PARSE(parse_expression, &list, name, "Expect List identifier after 'in' in foreach loop.");

        //initializer
        Token idx_token = make_token(TOKEN_IDENTIFIER, -1, "_idx_", 5);
        Token zero_token = make_token(TOKEN_INT, -1, "0", 1);
        struct Node* initializer = make_decl_var(idx_token, make_int_type(), make_literal(zero_token));

        //condition 
        Token prop_token = make_token(TOKEN_IDENTIFIER, -1, "size", 4);
        Token less_token = make_token(TOKEN_LESS, -1, "<", 1);
        struct Node* get_idx = make_get_var(idx_token);
        struct Node* get_size = make_get_prop(list, prop_token);
        struct Node* condition = make_logical(less_token, get_idx, get_size);

        //update
        Token plus_token = make_token(TOKEN_PLUS, -1, "+", 1);
        Token one_token = make_token(TOKEN_INT, -1, "1", 1);
        struct Node* sum = make_binary(plus_token, get_idx, make_literal(one_token));
        struct Node* update = make_expr_stmt(make_set_var(get_idx, sum)); 

        //body
        struct Node* get_element = make_get_element(make_dummy_token(), list, get_idx);
        ((DeclVar*)element)->right = get_element;

        //prepending element to body
        struct NodeList* body = (struct NodeList*)make_node_list();
        add_node(body, element);
        struct Node* remaining_body;
        if (declaration(&remaining_body) == RESULT_FAILED) {
            ERROR(name, "Expected body statement for 'foreach' loop.");
        }
        add_node(body, remaining_body);
        struct Node* then_block = make_block(name, body);

        *node = make_for(name, initializer, condition, update, then_block);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_RIGHT_ARROW)) {
        Token name = parser.previous;
        struct Node* right;
        PARSE(parse_sequence, &right, name, "Return value must be and expression or 'nil'.");
        *node = make_return(name, right);
        return RESULT_SUCCESS;
    }

    struct Node* seq;
    PARSE(parse_sequence, &seq, parser.previous, "Invalid statement.");
    *node = make_expr_stmt(seq);
    return RESULT_SUCCESS;
}

static void init_parser(struct Table* globals) {
    parser.errors = (struct Error*)malloc(MAX_ERRORS * sizeof(struct Error));
    parser.error_count = 0;
    parser.globals = globals;
    parser.statics_nl = (struct NodeList*)make_node_list();
    parser.import_count = 0;
}

static void free_parser() {
    for (int i = 0; i < parser.error_count; i++) {
        FREE_ARRAY(parser.errors[i].message, char, 100);
    }
    free(parser.errors);
}

static void prime_lexer(const char* source) {
    init_lexer(source);
    parser.current = next_token();
    parser.next = next_token();
    parser.next_next = next_token();
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

//TODO: this can be simplified to just passing in a struct Type** type, and casting to TypeIdentifer* and setting *type = resolved_type
static ResultCode resolve_identifier(struct TypeIdentifier* ti, struct Table* globals, struct Type** type) {
    ResultCode result = RESULT_SUCCESS;
    struct ObjString* identifier = make_string(ti->identifier.start, ti->identifier.length);
    push_root(to_string(identifier));
    Value val;
    PARSE_ERROR_IF(!get_entry(globals, identifier, &val), ti->identifier, "Identifier for type not declared.");
    if (result == RESULT_SUCCESS) *type = val.as.type_type;
    pop_root();
    return result;
}

static ResultCode resolve_function_identifiers(struct TypeFun* ft, struct Table* globals);

static ResultCode check_global_circular_inheritance(struct Table* globals) {
    for (int i = 0; i < globals->capacity; i++) {
        struct Entry* entry = &globals->entries[i];
        if (entry->value.type != VAL_TYPE) continue;
        if (entry->value.as.type_type->type != TYPE_STRUCT) continue;
        struct Type* type = entry->value.as.type_type;
        struct TypeStruct* klass = (struct TypeStruct*)type;
        Token struct_name = klass->name;
        struct Type* current = klass->super;
        while (current != NULL) {
            struct TypeStruct* super = (struct TypeStruct*)current;
            Token super_name = super->name;
            if (same_token_literal(struct_name, super_name)) {
                ERROR(make_dummy_token(), "A struct cannot have a circular inheritance.");
            }
            current = super->super;
        }
    }
    return RESULT_SUCCESS;
}

static ResultCode copy_global_inherited_props(struct Table* globals) {
    for (int i = 0; i < globals->capacity; i++) {
        struct Entry* entry = &globals->entries[i];
        if (entry->value.type != VAL_TYPE) continue;
        if (entry->value.as.type_type->type != TYPE_STRUCT) continue;
        struct Type* type = entry->value.as.type_type;
        struct TypeStruct* klass = (struct TypeStruct*)type; //this is the substruct we want to copy all props into
        struct Type* super_type = klass->super;
        while (super_type != NULL) {
            struct TypeStruct* super = (struct TypeStruct*)super_type;
            for (int j = 0; j < super->props.capacity; j++) {
                struct Entry* entry = &super->props.entries[j];
                if (entry->key == NULL) continue;
                Value val;
                if (get_entry(&klass->props, entry->key, &val)) {
                    if (val.as.type_type->type != TYPE_INFER && 
                        entry->value.as.type_type->type != TYPE_INFER && 
                        !same_type(val.as.type_type, entry->value.as.type_type)) {
                        ERROR(make_dummy_token(), "Overwritten properties must share same type.");
                    }
                } else {
                    set_entry(&klass->props, entry->key, entry->value);
                }
            }
            super_type = super->super;
        }
    }
    return RESULT_SUCCESS;
}

static ResultCode add_struct_by_order(struct NodeList* nl, struct Table* struct_set, struct DeclStruct* dc, struct NodeList* statics_nl) {
    if (dc->super != NULL) {
        GetVar* gv = (GetVar*)(dc->super);
        //iterate through statics_nl to find correct struct DeclStruct
        for (int i = 0; i < statics_nl->count; i++) {
            struct Node* n = statics_nl->nodes[i];
            if (n->type != NODE_STRUCT) continue;
            struct DeclStruct* super = (struct DeclStruct*)n;
            if (same_token_literal(gv->name, super->name)) {
                add_struct_by_order(nl, struct_set, super, statics_nl);
                break;
            }
        }
    }
    //make string from dc->name
    struct ObjString* klass_name = make_string(dc->name.start, dc->name.length);
    push_root(to_string(klass_name)); //calling pop_root() to clear strings in parse() after root call on add_struct_by_order
    Value val;
    if (!get_entry(struct_set, klass_name, &val)) {
        add_node(nl, (struct Node*)dc);
        set_entry(struct_set, klass_name, to_nil());
    }
    pop_root();
    return RESULT_SUCCESS;
}

static ResultCode resolve_function_identifiers(struct TypeFun* ft, struct Table* globals) {
    //check parameters
    struct TypeArray* params = (ft->params);
    for (int i = 0; i < params->count; i++) {
        struct Type** param_type = &params->types[i];
        if (resolve_type_identifiers(param_type, globals) == RESULT_FAILED) return RESULT_FAILED;
    }

    //check return
    for (int i = 0; i < ft->returns->count; i++) {
        struct Type** ret = &ft->returns->types[i];
        if (resolve_type_identifiers(ret, globals) == RESULT_FAILED) return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}

static ResultCode resolve_global_function_identifiers(struct Table* globals) {
    for (int i = 0; i < globals->capacity; i++) {
        struct Entry* entry = &globals->entries[i];
        if (entry->value.type != VAL_TYPE) continue;
        if (entry->value.as.type_type->type != TYPE_FUN) continue;
        struct TypeFun* fun_type = (struct TypeFun*)(entry->value.as.type_type);
        if (resolve_function_identifiers(fun_type, parser.globals) == RESULT_FAILED) return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}

static ResultCode resolve_global_struct_identifiers(struct Table* globals) {
    for (int i = 0; i < globals->capacity; i++) {
        struct Entry* entry = &globals->entries[i];
        if (entry->value.type != VAL_TYPE || entry->value.as.type_type->type != TYPE_STRUCT) continue;
        struct TypeStruct* tc = (struct TypeStruct*)(entry->value.as.type_type);
        //resolve properties
        for (int j = 0; j < tc->props.capacity; j++) {
            struct Entry* inner_entry = &tc->props.entries[j];
            if (inner_entry->value.type != VAL_TYPE) continue;
            if (inner_entry->value.as.type_type->type == TYPE_IDENTIFIER) {
                struct Type* result;
                if (resolve_identifier((struct TypeIdentifier*)(inner_entry->value.as.type_type), globals, &result) == RESULT_FAILED) return RESULT_FAILED;
                set_entry(&tc->props, inner_entry->key, to_type(result));
            } else if (inner_entry->value.as.type_type->type == TYPE_FUN) {
                if (resolve_function_identifiers((struct TypeFun*)(inner_entry->value.as.type_type), globals) == RESULT_FAILED) return RESULT_FAILED;
            }
        }
        //resolve structs inherited from
        if (tc->super == NULL) continue;
        if (resolve_type_identifiers(&tc->super, globals) == RESULT_FAILED) return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}

static ResultCode order_nodes_by_enums_structs_functions(struct NodeList* original, struct NodeList* ordered) {
    //add enums first in compilation order
    for (int i = 0; i < original->count; i++) {
        struct Node* n = original->nodes[i];
        if (n->type == NODE_ENUM) {
            add_node(ordered, n);
        }
    }

    //add structs next
    //make temp table inside ObjEnum so that GC doesn't sweep it
    struct ObjEnum* struct_set_wrapper = make_enum(make_dummy_token());
    push_root(to_enum(struct_set_wrapper));
    for (int i = 0; i < original->count; i++) {
        struct Node* n = original->nodes[i];
        if (n->type == NODE_STRUCT) {
            add_struct_by_order(ordered, &struct_set_wrapper->props, (struct DeclStruct*)n, original);
        }
    }
    pop_root();

    //add functions last
    for (int i = 0; i < original->count; i++) {
        struct Node* n = original->nodes[i];
        if (n->type == NODE_FUN) {
            add_node(ordered, n);
        }
    }

    return RESULT_SUCCESS;
}

//script compiler keeps a linked list of all AST nodes to delete when it's freed
//we're using it here to resolve any identifiers for user defined types in the entire list
static ResultCode resolve_remaining_identifiers(struct Table* globals, struct Node* node) {
    while (node != NULL) {
        switch(node->type) {
            case NODE_DECL_VAR: {
                DeclVar* dv = (DeclVar*)node;
                if (dv->type == NULL) break;
                if (resolve_type_identifiers(&dv->type, globals) == RESULT_FAILED) return RESULT_FAILED;
                break;
            }
            case NODE_CONTAINER: {
                struct DeclContainer* dc = (struct DeclContainer*)node;
                if (resolve_type_identifiers(&dc->type, globals) == RESULT_FAILED) return RESULT_FAILED;
                break;
            }
            case NODE_CAST: {
                Cast* c = (Cast*)node;
                if (c->type == NULL) break;
                if (resolve_type_identifiers(&c->type, globals) == RESULT_FAILED) return RESULT_FAILED;
                break;
            }
        }
        node = node->next;
    }
    return RESULT_SUCCESS;
}

static ResultCode resolve_type_identifiers(struct Type** type, struct Table* globals) {
    ResultCode result = RESULT_SUCCESS;

    switch((*type)->type) {
        case TYPE_IDENTIFIER: {
            if (resolve_identifier((struct TypeIdentifier*)(*type), globals, type) == RESULT_FAILED)
                result = RESULT_FAILED;
            break;
        }
        case TYPE_FUN: {
            struct TypeFun* tf = (struct TypeFun*)(*type);
            if (resolve_function_identifiers(tf, globals) == RESULT_FAILED)
                result = RESULT_FAILED;
            break;
        }
        case TYPE_LIST: {
            struct TypeList* tl = (struct TypeList*)(*type);
            if (resolve_type_identifiers(&tl->type, globals) == RESULT_FAILED)
                result = RESULT_FAILED;
            break;
        }
        case TYPE_MAP: {
           struct TypeMap* tm = (struct TypeMap*)(*type);
           if (resolve_type_identifiers(&tm->type, globals) == RESULT_FAILED)
               result = RESULT_FAILED;
           break;
        }
    }
    return result;
}

ResultCode parse_module(const char* source, struct NodeList* dynamic_nodes, struct Table* globals) {
    prime_lexer(source);
    ResultCode result = RESULT_SUCCESS;

    while(parser.current.type != TOKEN_EOF) {
        struct Node* decl;
        if (peek_one(TOKEN_IMPORT)) {
            if (parse_import() == RESULT_FAILED) {
                result = RESULT_FAILED;
                synchronize();
            }
        } else {
            if ((result = declaration(&decl)) == RESULT_SUCCESS) {
                add_to_compile_pass(decl, parser.statics_nl, dynamic_nodes);
            } else {
                result = RESULT_FAILED;
                synchronize();
            }
        }
    }

    return result;
}

//this is in main.c
ResultCode read_file(const char* path, const char** source);

ResultCode parse(const char* source, struct NodeList** final_ast, struct Table* globals, struct Node** all_nodes, struct ObjString* script_path) {
    //copy globals table so it can be reset if error occurs in repl
    struct Table copy;
    init_table(&copy);
    copy_table(&copy, globals);

    init_parser(globals);

    struct NodeList* script_nl = (struct NodeList*)make_node_list();
    ResultCode result = parse_module(source, script_nl, globals);

    //get path of script
    char* last_slash = strrchr(script_path->chars, DIR_SEPARATOR);
    int dir_len = last_slash == NULL ? 0 : last_slash - script_path->chars + 1;

    for (int i = 0; i < parser.import_count; i++) {
        Token import_name = parser.imports[i];
        char* path = (char*)malloc(dir_len + import_name.length + 5); //current script directory + module name + '.cbr' extension and null terminator
        memcpy(path, script_path->chars, dir_len);
        memcpy(path + dir_len, import_name.start, import_name.length);
        memcpy(path + dir_len + import_name.length, ".cbr\0", 5);
        const char* module_source;
        if (read_file(path, &module_source) == RESULT_FAILED) {
            printf("[Cebra Error] Module not found.\n");
            result = RESULT_FAILED;
        }
        if (result != RESULT_FAILED)
            result = parse_module(module_source, script_nl, globals);

        free(path);
    }

    if (result != RESULT_FAILED) result = resolve_global_struct_identifiers(parser.globals);
    if (result != RESULT_FAILED) result = check_global_circular_inheritance(parser.globals);
    if (result != RESULT_FAILED) result = copy_global_inherited_props(parser.globals);
    if (result != RESULT_FAILED) result = resolve_global_function_identifiers(parser.globals);
    if (result != RESULT_FAILED) result = resolve_remaining_identifiers(parser.globals, *all_nodes);

    struct NodeList* ordered_nl = (struct NodeList*)make_node_list();
    if (result != RESULT_FAILED) result = order_nodes_by_enums_structs_functions(parser.statics_nl, ordered_nl);

    for (int i = 0; i < script_nl->count; i++) {
        add_node(ordered_nl, script_nl->nodes[i]);
    }
    *final_ast = ordered_nl;

    if (parser.error_count > 0) {
        quick_sort(parser.errors, 0, parser.error_count - 1);
        for (int i = 0; i < parser.error_count; i++) {
            printf("Parse Error: [line %d] ", parser.errors[i].token.line);
            printf("%s\n", parser.errors[i].message);
        }
        if (parser.error_count == 256) {
            printf("Parsing error count exceeded maximum of 256.\n");
        }

        //don't need to free errors since parser is freed anyway
        copy_table(globals, &copy);
        free_table(&copy);
        
        free_parser();
        return RESULT_FAILED;
    }


    free_table(&copy);
    return result;
}

