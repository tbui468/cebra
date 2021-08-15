#ifndef CEBRA_AST_TYPER_H
#define CEBRA_AST_TYPER_H

#include "token.h"
#include "result_code.h"
#include "node_list.h"

typedef struct {
    Token token;
    const char* message;
} TypeError;

typedef struct {
    TypeError errors[256];
    int error_count; 
} Typer;

typedef enum {
    DATA_INT,
    DATA_FLOAT,
    DATA_STRING,
} DataType;

ResultCode type_check(NodeList* nl);


#endif// CEBRA_AST_TYPER_H
