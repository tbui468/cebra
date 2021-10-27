#ifndef CEBRA_ERROR_H
#define CEBRA_ERROR_H

#include "token.h"

struct Error {
    Token token;
    const char* message;
};

#endif //CEBRA_ERROR_H
