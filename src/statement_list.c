#include "statement_list.h"


void init_statement_list(StatementList* sl) {
    sl->stmts = ALLOCATE_ARRAY(Expr*);
    sl->count = 0;
    sl->capacity = 0;
}

void free_statement_list(StatementList* sl) {
    for (int i = 0; i < sl->count; i++) {
        free_expr(sl->stmts[i]);
    }
    FREE(sl->stmts);
}

void add_statement(StatementList* sl, Expr* expr) {
    if (sl->count + 1 > sl->capacity) {
        int new_capacity = sl->capacity == 0 ? 8 : sl->capacity * 2;
        sl->stmts = GROW_ARRAY(Expr*, sl->stmts, new_capacity);
        sl->capacity = new_capacity;
    }

    sl->stmts[sl->count] = expr;
    sl->count++;
}


void print_statement_list(StatementList* sl) {
    for (int i = 0; i < sl->count; i++) {
        print_expr(sl->stmts[i]);
    }
}
