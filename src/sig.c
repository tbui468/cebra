#include <stdio.h>
#include <string.h>
#include "sig.h"
#include "memory.h"

void init_sig_list(SigList* sl) {
    sl->sigs = ALLOCATE_ARRAY(struct Sig*);
    sl->count = 0;
    sl->capacity = 0;
}

void free_sig_list(SigList* sl) {
    for (int i = 0; i < sl->count; i++) {
        free_sig(sl->sigs[i]);
    }
    FREE_ARRAY(sl->sigs, struct Sig*, sl->capacity);
}


void add_sig(SigList* sl, struct Sig* sig) {
    if (sl->count + 1 > sl->capacity) {
        int new_capacity = sl->capacity == 0 ? 8 : sl->capacity * 2;
        sl->sigs = GROW_ARRAY(sl->sigs, struct Sig*, new_capacity, sl->capacity);
        sl->capacity = new_capacity;
    }

    sl->sigs[sl->count] = sig;
    sl->count++;
}

struct Sig* make_prim_sig(ValueType type) {
    SigPrim* sig_prim = ALLOCATE_NODE(SigPrim);

    sig_prim->base.type = SIG_PRIM;
    sig_prim->type = type;
    
    return (struct Sig*)sig_prim;
}

struct Sig* make_fun_sig(SigList params, struct Sig* ret) {
    SigFun* sig_fun = ALLOCATE_NODE(SigFun);

    sig_fun->base.type = SIG_FUN;
    sig_fun->params = params;
    sig_fun->ret = ret;
    
    return (struct Sig*)sig_fun;
}

struct Sig* make_class_sig(Token klass) {
    SigClass* sc = ALLOCATE_NODE(SigClass);

    sc->base.type = SIG_CLASS;
    sc->klass = klass;

    return (struct Sig*)sc;
}

bool same_sig_list(SigList* sl1, SigList* sl2) {
    if (sl1->count != sl2->count) return false;

    for (int i = 0; i < sl1->count; i++) {
        if (!same_sig(sl1->sigs[i], sl2->sigs[i])) return false;
    }

    return true;
}

bool same_sig(struct Sig* sig1, struct Sig* sig2) {
    if (sig1 == NULL || sig2 == NULL) return false;
    if (sig1->type != sig2->type) return false;

    switch(sig1->type) {
        case SIG_PRIM:
            return ((SigPrim*)sig1)->type == ((SigPrim*)sig2)->type;
        case SIG_FUN:
            bool ret_same = same_sig(((SigFun*)sig1)->ret, ((SigFun*)sig2)->ret);
            bool params_same = same_sig_list(&((SigFun*)sig1)->params, &((SigFun*)sig2)->params);
            return ret_same && params_same;
        case SIG_CLASS:
            SigClass* sc1 = (SigClass*)sig1;
            SigClass* sc2 = (SigClass*)sig2;
            if (sc1->klass.length != sc2->klass.length) return false;
            return memcmp(sc1->klass.start, sc2->klass.start, sc1->klass.length) == 0;
    }
}

struct Sig* copy_sig(struct Sig* sig);

static SigList copy_sig_list(SigList* sl) {
    SigList copy;
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
            SigClass* sc = (SigClass*)sig;
            return make_class_sig(copy_token(sc->klass));
    }
}

bool sig_is_type(struct Sig* sig, ValueType type) {
    if (sig->type != SIG_PRIM) return false;

    return ((SigPrim*)sig)->type == type;
}

void print_sig(struct Sig* sig) {
    switch(sig->type) {
        case SIG_PRIM:
            SigPrim* sp = (SigPrim*)sig;
            printf(" %s ", value_type_to_string(sp->type));
            break;
        case SIG_FUN:
            SigFun* sf = (SigFun*)sig;
            SigList* params = &sf->params;
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
        case SIG_PRIM:
            SigPrim* sig_prim = (SigPrim*)sig;
            FREE_NODE(sig_prim, SigPrim);
            break;
        case SIG_FUN:
            SigFun* sig_fun = (SigFun*)sig;
            free_sig_list(&sig_fun->params);
            free_sig(sig_fun->ret);
            FREE_NODE(sig_fun, SigFun);
            break;
        case SIG_CLASS:
            SigClass* sc = (SigClass*)sig;
            FREE_NODE(sc, SigClass);
            break;
    }
}
