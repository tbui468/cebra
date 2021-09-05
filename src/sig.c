#include <stdio.h>
#include <string.h>
#include "sig.h"
#include "memory.h"
#include "compiler.h"

void insert_sig(struct Sig* sig) {
    sig->next = current_compiler->signatures;
    current_compiler->signatures = sig;    
}

void add_sig(struct SigList* sl, struct Sig* sig) {
    if (sl->count + 1 > sl->capacity) {
        int new_capacity = sl->capacity == 0 ? 8 : sl->capacity * 2;
        sl->sigs = GROW_ARRAY(sl->sigs, struct Sig*, new_capacity, sl->capacity);
        sl->capacity = new_capacity;
    }

    sl->sigs[sl->count] = sig;
    sl->count++;
}

struct Sig* make_list_sig() {
    struct SigList* sig_list = ALLOCATE(struct SigList);
    sig_list->sigs = ALLOCATE_ARRAY(struct Sig*);
    sig_list->count = 0;
    sig_list->capacity = 0;

    sig_list->base.type = SIG_LIST;

    insert_sig((struct Sig*)sig_list);
    return (struct Sig*)sig_list;
}

struct Sig* make_prim_sig(ValueType type) {
    struct SigPrim* sig_prim = ALLOCATE(struct SigPrim);

    sig_prim->base.type = SIG_PRIM;
    sig_prim->type = type;
    
    insert_sig((struct Sig*)sig_prim);
    return (struct Sig*)sig_prim;
}

struct Sig* make_fun_sig(struct Sig* params, struct Sig* ret) {
    struct SigFun* sig_fun = ALLOCATE(struct SigFun);

    sig_fun->base.type = SIG_FUN;
    sig_fun->params = params;
    sig_fun->ret = ret;
    
    insert_sig((struct Sig*)sig_fun);
    return (struct Sig*)sig_fun;
}

struct Sig* make_class_sig() {
    struct SigClass* sc = ALLOCATE(struct SigClass);

    sc->base.type = SIG_CLASS;
    init_table(&sc->props);

    insert_sig((struct Sig*)sc);
    return (struct Sig*)sc;
}

bool is_duck(struct SigClass* sub, struct SigClass* super) {
    //grab each key in sub, and check if inside super (using tables as hash set rather than map)
    //no quick way of doing this since the tables are different, so we can't just use find_entry
    //to go directly to the correct bucket.
    return false;
}

bool same_sig(struct Sig* sig1, struct Sig* sig2) {
    if (sig1 == NULL || sig2 == NULL) return false;
    if (sig1->type != sig2->type) return false;

    switch(sig1->type) {
        case SIG_LIST:
            struct SigList* sl1 = (struct SigList*)sig1;
            struct SigList* sl2 = (struct SigList*)sig2;
            if (sl1->count != sl2->count) return false;
            for (int i = 0; i < sl1->count; i++) {
                if (!same_sig(sl1->sigs[i], sl2->sigs[i])) return false;
            }
            return true;
        case SIG_PRIM:
            return ((struct SigPrim*)sig1)->type == ((struct SigPrim*)sig2)->type;
        case SIG_FUN:
            struct SigFun* sf1 = (struct SigFun*)sig1;
            struct SigFun* sf2 = (struct SigFun*)sig2;
            return same_sig(sf1->ret, sf2->ret) && same_sig(sf1->params, sf2->params);
        case SIG_CLASS:
            struct SigClass* sc1 = (struct SigClass*)sig1;
            struct SigClass* sc2 = (struct SigClass*)sig2;
            return is_duck(sc1, sc2) && is_duck(sc2, sc1);
    }
}

/*
struct Sig* copy_sig(struct Sig* sig);

static struct SigList copy_sig_list(struct SigList* sl) {
    struct SigList copy;
    init_sig_list(&copy);
    for (int i = 0; i < sl->count; i++) {
        add_sig(&copy, copy_sig(sl->sigs[i]));
    }
    return copy;
}

struct Sig* copy_sig(struct Sig* sig) {
    switch(sig->type) {
        case SIG_PRIM:
            SigPrim* sp = (SigPrim*)sig;
            return make_prim_sig(sp->type);
        case SIG_FUN:
            SigFun* sf = (SigFun*)sig;
            return make_fun_sig(copy_sig_list(&sf->params), copy_sig(sf->ret));
        case SIG_CLASS:
            
           // SigClass* sc = (SigClass*)sig;
            //Table 
            //Sig* copy = make_class_sig
            return NULL;
    }
}*/

bool sig_is_type(struct Sig* sig, ValueType type) {
    if (sig->type != SIG_PRIM) return false;

    return ((struct SigPrim*)sig)->type == type;
}

void print_sig(struct Sig* sig) {
    switch(sig->type) {
        case SIG_LIST:
            printf("SigList Stub");
            break;
        case SIG_PRIM:
            struct SigPrim* sp = (struct SigPrim*)sig;
            printf(" %s ", value_type_to_string(sp->type));
            break;
        case SIG_FUN:
            struct SigFun* sf = (struct SigFun*)sig;
            struct SigList* params = (struct SigList*)(sf->params);
            printf("(");
            for (int i = 0; i < params->count; i++) {
                print_sig(params->sigs[i]);
            } 
            printf(") -> ");
            print_sig(sf->ret);
            break;
        case SIG_CLASS:
            printf("SigClass Stub");
            break;
    }
}

void free_sig(struct Sig* sig) {
    switch(sig->type) {
        case SIG_LIST:
            struct SigList* sl = (struct SigList*)sig;
            FREE_ARRAY(sl->sigs, struct Sig*, sl->capacity);
            FREE(sl, struct SigList);
            break;
        case SIG_PRIM:
            struct SigPrim* sig_prim = (struct SigPrim*)sig;
            FREE(sig_prim, struct SigPrim);
            break;
        case SIG_FUN:
            struct SigFun* sig_fun = (struct SigFun*)sig;
            FREE(sig_fun, struct SigFun);
            break;
        case SIG_CLASS:
            struct SigClass* sc = (struct SigClass*)sig;
            free_table(&sc->props);
            FREE(sc, struct SigClass);
            break;
    }
}
