#include "sig.h"
#include "memory.h"

//static Sig* copy_sig(Sig* sig);

void init_sig_list(SigList* sl) {
    sl->sigs = ALLOCATE_ARRAY(Sig*);
    sl->count = 0;
    sl->capacity = 0;
}

void free_sig_list(SigList* sl) {
    FREE_ARRAY(sl->sigs, Sig*, sl->capacity);
}


void add_sig(SigList* sl, Sig* sig) {
    if (sl->count + 1 > sl->capacity) {
        int new_capacity = sl->capacity == 0 ? 8 : sl->capacity * 2;
        sl->sigs = GROW_ARRAY(sl->sigs, Sig*, new_capacity, sl->capacity);
        sl->capacity = new_capacity;
    }

    sl->sigs[sl->count] = sig;
    sl->count++;
}

/*
SigList copy_sig_list(SigList* sl) {
    SigList copy;
    init_sig_list(&copy);

    for (int i = 0; i < sl->count; i++) {
        add_sig(&copy, copy_sig(sl->sigs[i]));
    }

    return copy;
}*/

Sig* make_prim_sig(ValueType type) {
    SigPrim* sig_prim = ALLOCATE_NODE(SigPrim);

    sig_prim->base.type = SIG_PRIM;
    sig_prim->type = type;
    
    return (Sig*)sig_prim;
}

Sig* make_fun_sig(SigList params, Sig* ret) {
    SigFun* sig_fun = ALLOCATE_NODE(SigFun);

    sig_fun->base.type = SIG_FUN;
    sig_fun->params = params;
    sig_fun->ret = ret;
    
    return (Sig*)sig_fun;
}

/*
static Sig* copy_sig(Sig* sig) {
    switch(sig->type) {
        case SIG_PRIM:
            SigPrim* sp = (SigPrim*)sig;
            return make_prim_sig(sp->type);
        case SIG_FUN:
            SigFun* sf = (SigFun*)sig;
            return make_fun_sig(copy_sig_list(&sf->params), copy_sig(sf->ret));
    }
}*/

void free_sig(Sig* sig) {
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
    }
}
