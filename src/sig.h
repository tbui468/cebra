#ifndef CEBRA_SIG_H
#define CEBRA_SIG_H

#include "value.h"
#include "table.h"

typedef enum {
    SIG_LIST,
    SIG_PRIM,
    SIG_FUN,
    SIG_CLASS,
    SIG_IDENTIFIER
} SigType;

struct Sig {
    SigType type;
    struct Sig* next;
    struct Sig* opt;
};

struct SigList {
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
};

void insert_sig(struct Sig* sig);

struct Sig* make_list_sig();
void add_sig(struct SigList* sl, struct Sig* sig);
struct Sig* make_prim_sig(ValueType type);
struct Sig* make_fun_sig(struct Sig* params, struct Sig* ret);
struct Sig* make_class_sig(Token klass);
struct Sig* make_identifier_sig(Token identifier);

bool is_duck(struct SigClass* sub, struct SigClass* super);
bool same_sig(struct Sig* sig1, struct Sig* sig2);
bool sig_is_type(struct Sig* sig, ValueType type);
void print_sig(struct Sig* sig);

struct Sig* copy_sig(struct Sig* sig);
void free_sig(struct Sig* sig);


#endif// CEBRA_SIG_H
