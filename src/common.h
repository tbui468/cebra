#ifndef CEBRA_COMMON_H
#define CEBRA_COMMON_H

#ifdef _WIN64
    #define DIR_SEPARATOR '\\'
#elif _WIN32
    #define DIR_SEPARATOR '\\'
#else
    #define DIR_SEPARATOR '/'
#endif


#define _CRT_SECURE_NO_WARNINGS //to disable warning about using fopen
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

//#define DEBUG_DISASSEMBLE
//#define DEBUG_TRACE
//#define DEBUG_AST
#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC


#define MAX_ERRORS 128
#define UINT8_COUNT (UINT8_MAX + 1)

#endif// CEBRA_COMMON_H
