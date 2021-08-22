#ifndef CEBRA_SIG_H
#define CEBRA_SIG_H

#include "value.h"

typedef enum {
    SIG_PRIM,
    SIG_FUN,
} SigType;

typedef struct {
    SigType type;
} Sig;

typedef struct {
    Sig** sigs;
    int count;
    int capacity;
} SigList;

typedef struct {
    Sig base;
    ValueType type;
} SigPrim;

typedef struct {
    Sig base;
    SigList params;
    Sig* ret;
} SigFun;

typedef struct {
    Sig base;

} SigClass;

void init_sig_list(SigList* sl);
void free_sig_list(SigList* sl);
void add_sig(SigList* sl, Sig* sig);

Sig* make_prim_sig(ValueType type);
Sig* make_fun_sig(SigList params, Sig* ret);

bool same_sig(Sig* sig1, Sig* sig2);
bool sig_is_type(Sig* sig, ValueType type);
void print_sig(Sig* sig);

Sig* copy_sig(Sig* sig);
void free_sig(Sig* sig);


#endif// CEBRA_SIG_H
