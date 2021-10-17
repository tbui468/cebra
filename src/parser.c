#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "memory.h"

#define PARSE(fun, var_name, node_ptr, token, ...) \
    if (fun(var_name, node_ptr) == RESULT_FAILED) { \
        if (token.type != TOKEN_DUMMY) { \
            ERROR(token, __VA_ARGS__); \
        } \
        return RESULT_FAILED; \
    }

#define PARSE_WITHOUT_MSG(fun, var_name, node_ptr) \
    PARSE(fun, var_name, node_ptr, make_dummy_token(), "")

#define PARSE_TYPE(var_name, type_ptr, token, ...) \
    if (parse_type(var_name, type_ptr) == RESULT_FAILED) { \
        ERROR(token, __VA_ARGS__); \
    }

#define CONSUME(token_type, token, ...) \
    if (!match(token_type)) { \
        ERROR(token, __VA_ARGS__); \
    }

#define ERROR(tkn, ...) \
{ \
    if (parser.error_count < 256) { \
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


Parser parser;

static ResultCode parse_multiple_expressions(Token var_name, struct Node** node);
static ResultCode declaration(struct Node** node);
static ResultCode block(struct Node* prepend, struct Node** node);
static ResultCode parse_type(Token var_name, struct Type** type);
static ResultCode param_declaration(struct Node** node);
static ResultCode assignment(Token var_name, struct Node** node, int expected);
static ResultCode parse_single_expression(Token var_name, struct Node** node);

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


static ResultCode parse_function(Token var_name, struct Node** node, bool anonymous) {

    struct NodeList* params = (struct NodeList*)make_node_list();
    struct TypeArray* param_type_list = make_type_array();

    if (!match(TOKEN_RIGHT_PAREN)) {
        do {
            struct Node* var_decl;
            if (param_declaration(&var_decl) == RESULT_FAILED) return RESULT_FAILED;

            if (match(TOKEN_EQUAL)) {
                Token param_token = ((DeclVar*)var_decl)->name;
                ERROR(param_token, "Trying to assign parameter '%.*s'.  Function parameters cannot be assigned.", 
                        param_token.length, param_token.start);
            }

            DeclVar* vd = (DeclVar*)var_decl;
            add_type(param_type_list, vd->type);
            add_node(params, var_decl);
        } while (match(TOKEN_COMMA));
        CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after parameter list.");
    }

    CONSUME(TOKEN_RIGHT_ARROW, parser.previous, "Expect '->' after parameter list.");
    CONSUME(TOKEN_LEFT_PAREN, parser.previous, "Expect '(' before return type list.");
    struct TypeArray* ret_type_list = make_type_array();
    if (match(TOKEN_RIGHT_PAREN)) {
        add_type(ret_type_list, make_nil_type());
    } else {
        do {
            struct Type* single_ret_type;
            PARSE_TYPE(var_name, &single_ret_type, parser.previous, "Expect valid return type after function parameters.");
            add_type(ret_type_list, single_ret_type);
        } while(match(TOKEN_COMMA));
        CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after return type list.");
    }
    struct Type* fun_type = make_fun_type(param_type_list, ret_type_list);

    //create type and add to global types for regular functions (not anonymous)
    if (!anonymous) {
        struct ObjString* fun_string = make_string(var_name.start, var_name.length);
        push_root(to_string(fun_string));
        Value v;
        if (get_from_table(parser.globals, fun_string, &v)) {
            pop_root();
            ERROR(var_name, "The identifier for this function is already used.");
        }

        //set globals in parser (which compiler uses)
        set_table(parser.globals, fun_string, to_type(fun_type));
        pop_root();
    }

    CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before function body.");

    Token body_start = parser.previous;
    struct Node* body;
    if (block(NULL, &body) == RESULT_FAILED) {
        ERROR(body_start, "Expect '}' at end of function body starting at line %d.", body_start.line);
    }

    //fun_type is only necessary for anonymous functions 
    *node = make_decl_fun(var_name, params, fun_type, body, anonymous);
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
        CONSUME(TOKEN_LEFT_PAREN, identifier, "Create container using '()'.");
        CONSUME(TOKEN_RIGHT_PAREN, identifier, "Create container using '()'.");
        *node = make_decl_container(identifier, make_list_type(template_type)); 
        return RESULT_SUCCESS;
    } else if (match(TOKEN_MAP)) {
        Token identifier = parser.previous;
        CONSUME(TOKEN_LESS, identifier, "Expect '<' after 'Map'.");
        struct Type* template_type;
        PARSE_TYPE(var_name, &template_type, var_name, "Map must be initialized with valid type: Map<[type]>().");
        CONSUME(TOKEN_GREATER, identifier, "Expect '>' after type.");
        CONSUME(TOKEN_LEFT_PAREN, identifier, "Create container using '()'.");
        CONSUME(TOKEN_RIGHT_PAREN, identifier, "Create container using '()'.");
        *node = make_decl_container(identifier, make_map_type(template_type)); 
        return RESULT_SUCCESS;
    } else if (match(TOKEN_IDENTIFIER)) {
        Token id = parser.previous;
        if (match(TOKEN_COLON)) {
            struct Type* type;
            PARSE_TYPE(var_name, &type, var_name, "Expected valid type after ':'.");
            *node = make_decl_var(id, type, NULL);
            return RESULT_SUCCESS;
        } 
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
            if (parse_function(make_dummy_token(), node, true) == RESULT_FAILED) { 
                ERROR(var_name, "Invalid anonymous function declaration."); 
            }
            return RESULT_SUCCESS;
        }

        //check for group (expression in parentheses)
        struct Node* expr;
        PARSE(parse_single_expression, var_name, &expr, name, "Expect valid expression after '('.");

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
                        PARSE(parse_single_expression, var_name, &arg, parser.previous, "Function call argument must be a valid expression.");
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
                PARSE(parse_single_expression, var_name, &idx, left_bracket, "Expect string after '[' for Map access, or int for List access.");
                left = make_get_element(parser.previous, left, idx);
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

static ResultCode assignment(Token var_name, struct Node** node, int expected) {
    struct NodeList* node_sequence = (struct NodeList*)make_node_list();

    do {
        if (node_sequence->count > 255) {
            ERROR(parser.previous, "Maximun number of assignments/declarations separated by ',' is 256.");
        }
        struct Node* left;
        PARSE_WITHOUT_MSG(or, var_name, &left);
        add_node(node_sequence, left);
        if (expected != -1 && node_sequence->count >= expected) break;
    } while (match(TOKEN_COMMA));


    while (match(TOKEN_EQUAL) || match(TOKEN_COLON_EQUAL)) { 
        Token name = parser.previous;

        struct Node* assign_value;
        //TODO: parse with or() here and manually check for ',' to prevent a sequence within sequence
        //parse until ':=' or '=' or non ',' since we don't know how many will exists if functions with multiple
        //returns are included
        if (assignment(var_name, &assign_value, node_sequence->count) == RESULT_FAILED) { 
            ERROR(var_name, "Right hand side of '%.*s' must be a valid expression.", name.length, name.start);
            return RESULT_FAILED;
        }

        if (node_sequence->count != ((struct NodeList*)assign_value)->count) {
            ERROR(parser.previous, "Sequence lengths must match.");
        }
        
        for (int i = 0; i < node_sequence->count; i++) {
            struct Node* left = node_sequence->nodes[i];
            struct Node* right = ((struct NodeList*)assign_value)->nodes[i];

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

            node_sequence->nodes[i] = left;

        }
    }

    *node = (struct Node*)node_sequence;
    return RESULT_SUCCESS;
}

static ResultCode parse_single_expression(Token var_name, struct Node** node) {
    if (assignment(var_name, node, 1) == RESULT_FAILED) {
        //caller should add error
        return RESULT_FAILED;
    }
    *node = ((struct NodeList*)(*node))->nodes[0];
    return RESULT_SUCCESS;
}

static ResultCode parse_multiple_expressions(Token var_name, struct Node** node) {
    struct Node* result;
    if (assignment(var_name, &result, -1) == RESULT_FAILED) {
        return RESULT_FAILED;
    }

    if (result->type == NODE_LIST && ((struct NodeList*)result)->count == 1) {
        *node = ((struct NodeList*)result)->nodes[0];
        return RESULT_SUCCESS;
    }

    *node = result;
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
        struct Node* decl;
        if (declaration(&decl) == RESULT_SUCCESS) {
            add_to_compile_pass(decl, parser.first_pass_nl, body);
        } else {
            synchronize();
        }
    }
    *node = make_block(name, body);
    return RESULT_SUCCESS;
}

//Note: TOKEN_COLON is consume before if a variable declaration
static ResultCode parse_type(Token var_name, struct Type** type) {

    //for explicit function type declaration
    //TODO: fix bug here
    if (match(TOKEN_LEFT_PAREN)) {
        struct TypeArray* params = make_type_array();
        if (!match(TOKEN_RIGHT_PAREN)) {
            do {
                struct Type* param_type;
                PARSE_TYPE(var_name, &param_type, var_name, "Invalid parameter type for function declaration '%.*s'.", var_name.length, var_name.start);
                add_type(params, param_type);
            } while(match(TOKEN_COMMA));
            CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after parameter types.");
        }
        CONSUME(TOKEN_RIGHT_ARROW, parser.previous, "Expect '->' followed by return type.");
        CONSUME(TOKEN_LEFT_PAREN, parser.previous, "Expect '(' before return types.");
        //TODO: need to loop here and return typearray of returns
        struct TypeArray* returns = make_type_array();
        if (match(TOKEN_RIGHT_PAREN)) {
            add_type(returns, make_nil_type());
        } else {
            do {
                struct Type* ret_type;
                PARSE_TYPE(var_name, &ret_type, var_name, "Invalid return type for function declaration '%.*s'.", var_name.length, var_name.start);
                add_type(returns, ret_type);
            } while(match(TOKEN_COMMA));
            CONSUME(TOKEN_RIGHT_PAREN, parser.previous, "Expect ')' after return types.");
        }
        *type = make_fun_type(params, returns);
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

    if (match(TOKEN_STRUCT)) {
        if (match(TOKEN_LESS)) {
            CONSUME(TOKEN_IDENTIFIER, parser.previous, "Expect superclass identifier after '<'.");
            *type = make_struct_type(var_name, make_identifier_type(parser.previous));
            return RESULT_SUCCESS;
        }

        *type = make_struct_type(var_name, NULL);
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

    if (match(TOKEN_NIL)
            //|| (parser.previous.type == TOKEN_LEFT_PAREN && peek_one(TOKEN_RIGHT_PAREN))
            /*
            //function declaration: create nil return type
            (parser.previous.type == TOKEN_RIGHT_ARROW && peek_one(TOKEN_LEFT_BRACE)) ||
            //closure declaration: create nil return type
            (parser.previous.type == TOKEN_RIGHT_ARROW && peek_one(TOKEN_EQUAL))*/
            ) {
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

static ResultCode add_prop_to_struct_type(struct TypeStruct* tc, DeclVar* dv) {
    Token prop_name = dv->name;
    struct ObjString* prop_string = make_string(prop_name.start, prop_name.length); 
    push_root(to_string(prop_string));

    Value v;
    if (get_from_table(&tc->props, prop_string, &v)) {
        pop_root();
        ERROR(prop_name, "Field name already used once in this struct.");
    }

    set_table(&tc->props, prop_string, to_type(dv->type));
    pop_root();
    return RESULT_SUCCESS;
}

static ResultCode declaration(struct Node** node) {
    if (peek_three(TOKEN_IDENTIFIER, TOKEN_COLON_COLON, TOKEN_ENUM)) {
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

        //set globals in parser (which compiler uses for type checking)
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
    } else if (peek_three(TOKEN_IDENTIFIER, TOKEN_COLON_COLON, TOKEN_STRUCT)) {
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
        set_table(parser.globals, struct_string, to_type(struct_type));
        pop_root();

        struct Node* super = parser.previous.type == TOKEN_IDENTIFIER ? 
                             make_get_var(parser.previous) : 
                             NULL;

        CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect '{' before class body.");

        struct NodeList* nl = (struct NodeList*)make_node_list();
        while (!match(TOKEN_RIGHT_BRACE)) {
            struct Node* field;
            PARSE(parse_multiple_expressions, make_dummy_token(), &field, struct_name, "Expect field declarations inside struct body.");
            struct TypeStruct* tc = (struct TypeStruct*)struct_type;
            switch(field->type) {
                case NODE_DECL_VAR: {
                    DeclVar* dv = (DeclVar*)field;
                    if (add_prop_to_struct_type(tc, dv) == RESULT_FAILED) return RESULT_FAILED;
                    add_node(nl, (struct Node*)dv);
                    break;
                }
                case NODE_LIST: {
                    struct NodeList* decls = (struct NodeList*)field;
                    for (int i = 0; i < decls->count; i++) {
                        if (decls->nodes[i]->type != NODE_DECL_VAR) {
                            ERROR(struct_name, "Expecting field declarations and assignments.");
                        }
                        DeclVar* dv = (DeclVar*)(decls->nodes[i]);
                        if (add_prop_to_struct_type(tc, dv) == RESULT_FAILED) return RESULT_FAILED;
                        add_node(nl, (struct Node*)dv);
                    }
                    break;
                }
                default: {
                    ERROR(parser.previous, "Shit broke - expecting a NODE_LIST or NODE_DECL_VAR ast node.");
                    break;
                }
            }
        }

        *node = make_decl_struct(struct_name, super, nl);
        return RESULT_SUCCESS;
    } else if (peek_three(TOKEN_IDENTIFIER, TOKEN_COLON_COLON, TOKEN_LEFT_PAREN)) {
        match(TOKEN_IDENTIFIER);
        Token var_name = parser.previous;
        match(TOKEN_COLON_COLON);
        match(TOKEN_LEFT_PAREN);

        if (parse_function(var_name, node, false) == RESULT_FAILED) { 
            ERROR(var_name, "Invalid function declaration."); 
        }

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
        PARSE(parse_single_expression, make_dummy_token(), &condition, name, "Expect single boolean expression after 'if'.");

        CONSUME(TOKEN_LEFT_BRACE, parser.previous, "Expect boolean expression and '{' after 'if' condition.");

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
    } else if (match(TOKEN_WHEN)) {
        Token name = parser.previous;
        struct Node* left;
        PARSE(parse_single_expression, name, &left, name, "Expected a valid expression after 'when'.");
        CONSUME(TOKEN_LEFT_BRACE, name, "Expect '{' after 'when' and variable.");
        struct NodeList* cases = (struct NodeList*)make_node_list();

        while (match(TOKEN_IS)) {
            Token is = parser.previous;

            struct Node* right;
            PARSE(parse_single_expression, is, &right, is, "Expected expression to test equality with variable after 'when'.");
            Token make_token(TokenType type, int line, const char* start, int length);
            Token equal = make_token(TOKEN_EQUAL_EQUAL, is.line, "==", 2);
            struct Node* condition = make_logical(equal, left, right);

            CONSUME(TOKEN_LEFT_BRACE, name, "Expected '{' after 'is' and expression.");
            struct Node* body = NULL;
            if (block(NULL, &body) == RESULT_FAILED) {
                ERROR(name, "Expect close '}' after 'is' block body.");
            }
            struct Node* if_stmt = make_if_else(is, condition, body, NULL);
            add_node(cases, if_stmt);
        }

        //make_if_else(else token, true, block, NULL)
        if (match(TOKEN_ELSE)) {
            Token else_token = parser.previous;
            CONSUME(TOKEN_LEFT_BRACE, name, "Expected '{' after 'else'.");
            Token true_token = make_token(TOKEN_TRUE, else_token.line, "true", 4);
            struct Node* condition = make_literal(true_token);
            struct Node* body = NULL;
            if (block(NULL, &body) == RESULT_FAILED) {
                ERROR(name, "Expect close '}' after 'else' block body.");
            }
            add_node(cases, make_if_else(else_token, condition, body, NULL));
        }
        CONSUME(TOKEN_RIGHT_BRACE, name, "Expected '}' to close 'when' body.");

        *node = make_when(name, cases);
        return RESULT_SUCCESS;
    } else if (match(TOKEN_WHILE)) {
        Token name = parser.previous;
        struct Node* condition;
        PARSE(parse_single_expression, make_dummy_token(), &condition, name, "Expect condition after 'while'.");
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
            PARSE(parse_single_expression, make_dummy_token(), &initializer, name, "Expected initializer or empty space for first item in 'for' loop.");
            CONSUME(TOKEN_COMMA, name, "Expect ',' after for-loop initializer.");
        }

        struct Node* condition = NULL;
        if (!match(TOKEN_COMMA)) {
            PARSE(parse_single_expression, make_dummy_token(), &condition, name, "Expect update or empty space for third item in 'for' loop.");
            CONSUME(TOKEN_COMMA, name, "Expect ',' after for-loop condition.");
        }

        struct Node* update = NULL;
        if (!match(TOKEN_LEFT_BRACE)) {
            struct Node* update_expr;
            PARSE(parse_single_expression, make_dummy_token(), &update_expr, name, "Expect update or empty space for third item in 'for' loop.");
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
        PARSE(parse_single_expression, make_dummy_token(), &list, name, "Expect List identifier after 'in' in foreach loop.");

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
        PARSE(parse_multiple_expressions, make_dummy_token(), &right, name, "Return value must be and expression, or leave empty for 'nil' return.");
        *node = make_return(name, right);
        return RESULT_SUCCESS;
    }

    struct Node* expr;
    PARSE(parse_multiple_expressions, make_dummy_token(), &expr, parser.previous, "Invalid expression.");
    //TODO: if a DeclVar, don't pop it
    if (expr->type == NODE_LIST) {
        struct NodeList* list = (struct NodeList*)expr;
        for (int i = 0; i < list->count; i++) {
            if (list->nodes[i]->type != NODE_DECL_VAR) {
                list->nodes[i] = make_expr_stmt(list->nodes[i]);
            }
        }
        if (list->count == 1) {
            *node = list->nodes[0];
        } else {
            *node = (struct Node*)list;
        }
        return RESULT_SUCCESS;
    } else if (expr->type != NODE_DECL_VAR) {
        *node = make_expr_stmt(expr);
        return RESULT_SUCCESS;
    }

    *node = expr;
    return RESULT_SUCCESS;
}

static void init_parser(const char* source, struct Table* globals) {
    init_lexer(source);
    parser.current = next_token();
    parser.next = next_token();
    parser.next_next = next_token();
    parser.error_count = 0;
    parser.globals = globals;
    parser.first_pass_nl = (struct NodeList*)make_node_list();
    parser.resolve_id_list = (struct NodeList*)make_node_list();
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


static ResultCode resolve_identifier(struct TypeIdentifier* ti, struct Table* globals, struct Type** type) {
    struct ObjString* identifier = make_string(ti->identifier.start, ti->identifier.length);
    push_root(to_string(identifier));
    Value val;
    if (!get_from_table(globals, identifier, &val)) {
        pop_root();
        ERROR(ti->identifier, "Identifier for type not declared.");
    }
    pop_root();
    *type = val.as.type_type;
    return RESULT_SUCCESS;
}

static ResultCode resolve_list_identifiers(struct TypeList* tl, struct Table* globals) {
    if (tl->type->type == TYPE_IDENTIFIER) {
        struct Type* result;
        if (resolve_identifier((struct TypeIdentifier*)(tl->type), globals, &result) == RESULT_FAILED) return RESULT_FAILED;
        tl->type = result;
    }
    return RESULT_SUCCESS;
}

static ResultCode resolve_map_identifiers(struct TypeMap* tm, struct Table* globals) {
    if (tm->type->type == TYPE_IDENTIFIER) {
        struct Type* result;
        if (resolve_identifier((struct TypeIdentifier*)(tm->type), globals, &result) == RESULT_FAILED) return RESULT_FAILED;
        tm->type = result;
    }
    return RESULT_SUCCESS;
}

static ResultCode resolve_function_identifiers(struct TypeFun* ft, struct Table* globals);

static ResultCode check_global_circular_inheritance(struct Table* globals) {
    for (int i = 0; i < globals->capacity; i++) {
        struct Pair* pair = &globals->pairs[i];
        if (pair->value.type != VAL_TYPE) continue;
        if (pair->value.as.type_type->type != TYPE_STRUCT) continue;
        struct Type* type = pair->value.as.type_type;
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
        struct Pair* pair = &globals->pairs[i];
        if (pair->value.type != VAL_TYPE) continue;
        if (pair->value.as.type_type->type != TYPE_STRUCT) continue;
        struct Type* type = pair->value.as.type_type;
        struct TypeStruct* klass = (struct TypeStruct*)type; //this is the substruct we want to copy all props into
        struct Type* super_type = klass->super;
        while (super_type != NULL) {
            struct TypeStruct* super = (struct TypeStruct*)super_type;
            for (int j = 0; j < super->props.capacity; j++) {
                struct Pair* pair = &super->props.pairs[j];
                if (pair->key == NULL) continue;
                Value val;
                if (get_from_table(&klass->props, pair->key, &val)) {
                    if (val.as.type_type->type != TYPE_INFER && 
                        pair->value.as.type_type->type != TYPE_INFER && 
                        !same_type(val.as.type_type, pair->value.as.type_type)) {
                        ERROR(make_dummy_token(), "Overwritten properties must share same type.");
                    }
                } else {
                    set_table(&klass->props, pair->key, pair->value);
                }
            }
            super_type = super->super;
        }
    }
    return RESULT_SUCCESS;
}

static ResultCode add_struct_by_order(struct NodeList* nl, struct Table* struct_set, struct DeclStruct* dc, struct NodeList* first_pass_nl) {
    if (dc->super != NULL) {
        GetVar* gv = (GetVar*)(dc->super);
        //iterate through first_pass_nl to find correct struct DeclStruct
        for (int i = 0; i < first_pass_nl->count; i++) {
            struct Node* n = first_pass_nl->nodes[i];
            if (n->type != NODE_STRUCT) continue;
            struct DeclStruct* super = (struct DeclStruct*)n;
            if (same_token_literal(gv->name, super->name)) {
                add_struct_by_order(nl, struct_set, super, first_pass_nl);
                break;
            }
        }
    }
    //make string from dc->name
    struct ObjString* klass_name = make_string(dc->name.start, dc->name.length);
    push_root(to_string(klass_name)); //calling pop_root() to clear strings in parse() after root call on add_struct_by_order
    Value val;
    if (!get_from_table(struct_set, klass_name, &val)) {
        add_node(nl, (struct Node*)dc);
        set_table(struct_set, klass_name, to_nil());
    }
    pop_root();
    return RESULT_SUCCESS;
}

static ResultCode resolve_function_identifiers(struct TypeFun* ft, struct Table* globals) {
    //check parameters
    struct TypeArray* params = (ft->params);
    for (int i = 0; i < params->count; i++) {
        struct Type* param_type = params->types[i];
        if (param_type->type == TYPE_IDENTIFIER) {
            struct Type* result;
            if (resolve_identifier((struct TypeIdentifier*)param_type, globals, &result) == RESULT_FAILED) return RESULT_FAILED;
            params->types[i] = result;
        } else if (param_type->type == TYPE_FUN) {
            resolve_function_identifiers((struct TypeFun*)param_type, globals);
        } 
    }

    //check return
    for (int i = 0; i < ft->returns->count; i++) {
        struct Type* ret = ft->returns->types[i];
        if (ret->type == TYPE_IDENTIFIER) {
            struct Type* result;
            if (resolve_identifier((struct TypeIdentifier*)ret, parser.globals, &result) == RESULT_FAILED) return RESULT_FAILED;
            ft->returns->types[i] = result;
        } else if (ret->type == TYPE_FUN) {
            if (resolve_function_identifiers((struct TypeFun*)ret, globals) == RESULT_FAILED) return RESULT_FAILED;
        } else if (ret->type == TYPE_LIST) {
            if (resolve_list_identifiers((struct TypeList*)ret, globals) == RESULT_FAILED) return RESULT_FAILED;
        } else if (ret->type == TYPE_MAP) {
            if (resolve_map_identifiers((struct TypeMap*)ret, globals) == RESULT_FAILED) return RESULT_FAILED;
        }
    }

    return RESULT_SUCCESS;
}

static ResultCode resolve_global_function_identifiers(struct Table* globals) {
    for (int i = 0; i < globals->capacity; i++) {
        struct Pair* pair = &globals->pairs[i];
        if (pair->value.type != VAL_TYPE) continue;
        if (pair->value.as.type_type->type != TYPE_FUN) continue;
        struct TypeFun* fun_type = (struct TypeFun*)(pair->value.as.type_type);
        if (resolve_function_identifiers(fun_type, parser.globals) == RESULT_FAILED) return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}

static ResultCode resolve_global_struct_identifiers(struct Table* globals) {
    for (int i = 0; i < globals->capacity; i++) {
        struct Pair* pair = &globals->pairs[i];
        if (pair->value.type != VAL_TYPE || pair->value.as.type_type->type != TYPE_STRUCT) continue;
        struct TypeStruct* tc = (struct TypeStruct*)(pair->value.as.type_type);
        //resolve properties
        for (int j = 0; j < tc->props.capacity; j++) {
            struct Pair* inner_pair = &tc->props.pairs[j];
            if (inner_pair->value.type != VAL_TYPE) continue;
            if (inner_pair->value.as.type_type->type == TYPE_IDENTIFIER) {
                struct Type* result;
                if (resolve_identifier((struct TypeIdentifier*)(inner_pair->value.as.type_type), globals, &result) == RESULT_FAILED) return RESULT_FAILED;
                set_table(&tc->props, inner_pair->key, to_type(result));
            } else if (inner_pair->value.as.type_type->type == TYPE_FUN) {
                if (resolve_function_identifiers((struct TypeFun*)(inner_pair->value.as.type_type), globals) == RESULT_FAILED) return RESULT_FAILED;
            }
        }
        //resolve structs inherited from
        if (tc->super != NULL && tc->super->type == TYPE_IDENTIFIER)  {
            struct Type* result;
            if (resolve_identifier((struct TypeIdentifier*)(tc->super), globals, &result) == RESULT_FAILED) return RESULT_FAILED;
            tc->super = result;
        }
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

static ResultCode resolve_remaining_identifiers(struct Table* globals, struct Node* node) {
    while (node != NULL) {
        switch(node->type) {
            case NODE_DECL_VAR: {
                DeclVar* dv = (DeclVar*)node;
                if (dv->type == NULL) break;
                if (dv->type->type == TYPE_IDENTIFIER) {
                    struct Type* result;
                    if (resolve_identifier((struct TypeIdentifier*)(dv->type), globals, &result) == RESULT_FAILED) return RESULT_FAILED;
                    dv->type = result;
                }
                if (dv->type->type == TYPE_LIST) {
                    if (resolve_list_identifiers((struct TypeList*)(dv->type), globals) == RESULT_FAILED) return RESULT_FAILED;
                }
                if (dv->type->type == TYPE_MAP) {
                    if (resolve_map_identifiers((struct TypeMap*)(dv->type), globals) == RESULT_FAILED) return RESULT_FAILED;
                }
                break;
            }
            case NODE_CONTAINER: {
                struct DeclContainer* dc = (struct DeclContainer*)node;
                if (dc->type->type == TYPE_LIST) {
                    if (resolve_list_identifiers((struct TypeList*)(dc->type), globals) == RESULT_FAILED) return RESULT_FAILED;
                }
                if (dc->type->type == TYPE_MAP) {
                    if (resolve_map_identifiers((struct TypeMap*)(dc->type), globals) == RESULT_FAILED) return RESULT_FAILED;
                }
                break;
            }
            case NODE_CAST: {
                Cast* c = (Cast*)node;
                if (c->type == NULL) break;
                if (c->type->type == TYPE_IDENTIFIER) {
                    struct Type* result;
                    if (resolve_identifier((struct TypeIdentifier*)(c->type), globals, &result) == RESULT_FAILED) return RESULT_FAILED; 
                    c->type = result;
                }
                break;
            }
        }
        node = node->next;
    }
    return RESULT_SUCCESS;
}

ResultCode parse(const char* source, struct NodeList** nl, struct Table* globals, struct Node** all_nodes) {
    init_parser(source, globals);
    struct NodeList* script_nl = (struct NodeList*)make_node_list();

    ResultCode result = RESULT_SUCCESS;

    while(parser.current.type != TOKEN_EOF) {
        struct Node* decl;
        if (declaration(&decl) == RESULT_SUCCESS) {
            add_to_compile_pass(decl, parser.first_pass_nl, script_nl);
        } else {
            synchronize();
            result = RESULT_FAILED;
        }
    }

    if (result != RESULT_FAILED) result = resolve_global_struct_identifiers(parser.globals);
    if (result != RESULT_FAILED) result = check_global_circular_inheritance(parser.globals);
    if (result != RESULT_FAILED) result = copy_global_inherited_props(parser.globals);
    if (result != RESULT_FAILED) result = resolve_global_function_identifiers(parser.globals);
    if (result != RESULT_FAILED) result = resolve_remaining_identifiers(parser.globals, *all_nodes);

    struct NodeList* ordered_nl = (struct NodeList*)make_node_list();
    if (result != RESULT_FAILED) result = order_nodes_by_enums_structs_functions(parser.first_pass_nl, ordered_nl);

    for (int i = 0; i < script_nl->count; i++) {
        add_node(ordered_nl, script_nl->nodes[i]);
    }
    *nl = ordered_nl;

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

