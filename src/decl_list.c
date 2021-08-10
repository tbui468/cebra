#include "decl_list.h"


void init_decl_list(DeclList* dl) {
    dl->decls = ALLOCATE_ARRAY(Expr*);
    dl->count = 0;
    dl->capacity = 0;
}

void free_decl_list(DeclList* dl) {
    for (int i = 0; i < dl->count; i++) {
        free_expr(dl->decls[i]);
    }
    FREE(dl->decls);
}

void add_decl(DeclList* dl, Expr* expr) {
    if (dl->count + 1 > dl->capacity) {
        int new_capacity = dl->capacity == 0 ? 8 : dl->capacity * 2;
        dl->decls = GROW_ARRAY(Expr*, dl->decls, new_capacity);
        dl->capacity = new_capacity;
    }

    dl->decls[dl->count] = expr;
    dl->count++;
}


void print_decl_list(DeclList* dl) {
    for (int i = 0; i < dl->count; i++) {
        print_expr(dl->decls[i]);
        printf("\n");
    }
}
