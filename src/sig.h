#ifndef CEBRA_SIG_H
#define CEBRA_SIG_H

#include "value.h"

typedef enum {
    SIG_PRIM,
    SIG_FUN,
    SIG_CLASS
} SigType;

struct Sig {
    SigType type;
};

typedef struct {
    struct Sig** sigs;
    int count;
    int capacity;
} SigList;

typedef struct {
    struct Sig base;
    ValueType type;
} SigPrim;

typedef struct {
    struct Sig base;
    SigList params;
    struct Sig* ret;
} SigFun;

typedef struct {
    struct Sig base;
    Token klass; //not using 'class' in case we want to compile in c++
} SigClass;

void init_sig_list(SigList* sl);
void free_sig_list(SigList* sl);
void add_sig(SigList* sl, struct Sig* sig);

struct Sig* make_prim_sig(ValueType type);
struct Sig* make_fun_sig(SigList params, struct Sig* ret);
struct Sig* make_class_sig(Token klass);

bool same_sig(struct Sig* sig1, struct Sig* sig2);
bool sig_is_type(struct Sig* sig, ValueType type);
void print_sig(struct Sig* sig);

struct Sig* copy_sig(struct Sig* sig);
void free_sig(struct Sig* sig);


#endif// CEBRA_SIG_H
