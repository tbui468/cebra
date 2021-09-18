#ifndef CEBRA_SIG_H
#define CEBRA_SIG_H

#include "value.h"
#include "table.h"

typedef enum {
    SIG_ARRAY,
    SIG_PRIM,
    SIG_FUN,
    SIG_CLASS,
    SIG_IDENTIFIER,
    SIG_LIST
} SigType;

struct Sig {
    SigType type;
    struct Sig* next;
    struct Sig* opt;
};

struct SigArray {
    struct Sig base;
    struct Sig** sigs;
    int count;
    int capacity;
};

struct SigPrim {
    struct Sig base;
    ValueType type;
};

struct SigFun {
    struct Sig base;
    struct Sig* params;
    struct Sig* ret;
};

struct SigClass {
    struct Sig base;
    Token klass;
    struct Table props;
};

struct SigIdentifier {
    struct Sig base;
    Token identifier;
    struct Sig* type;
};

struct SigList {
    struct Sig base;
    struct Sig* type;
};


void insert_sig(struct Sig* sig);

struct Sig* make_array_sig();
void add_sig(struct SigArray* sl, struct Sig* sig);
struct Sig* make_prim_sig(ValueType type);
struct Sig* make_fun_sig(struct Sig* params, struct Sig* ret);
struct Sig* make_class_sig(Token klass);
struct Sig* make_identifier_sig(Token identifier, struct Sig* type);
struct Sig* make_list_sig(struct Sig* type);

bool is_duck(struct SigClass* sub, struct SigClass* super);
bool same_sig(struct Sig* sig1, struct Sig* sig2);
bool sig_is_type(struct Sig* sig, ValueType type);
void print_sig(struct Sig* sig);

struct Sig* copy_sig(struct Sig* sig);
void free_sig(struct Sig* sig);


#endif// CEBRA_SIG_H
