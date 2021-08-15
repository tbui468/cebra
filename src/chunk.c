#include "chunk.h"
#include "object.h"
#include "memory.h"

void init_chunk(Chunk* chunk) {
    chunk->codes = ALLOCATE_ARRAY(OpCode);
    chunk->count = 0;
    chunk->capacity = 0;
}

void free_chunk(Chunk* chunk) {
    FREE_ARRAY(chunk->codes, OpCode, chunk->capacity);
}

const char* op_to_string(OpCode op) {
    switch(op) {
        case OP_INT: return "OP_INT";
        case OP_FLOAT: return "OP_FLOAT";
        case OP_STRING: return "OP_STRING";
        case OP_FUN: return "OP_FUN";
        case OP_PRINT: return "OP_PRINT";
        case OP_SET_VAR: return "OP_SET_VAR";
        case OP_GET_VAR: return "OP_GET_VAR";
        case OP_ADD: return "OP_ADD";
        case OP_SUBTRACT: return "OP_SUBTRACT";
        case OP_MULTIPLY: return "OP_MULTIPLY";
        case OP_DIVIDE: return "OP_DIVIDE";
        case OP_MOD: return "OP_MOD";
        case OP_NEGATE: return "OP_NEGATE";
        case OP_POP: return "OP_POP";
        case OP_TRUE: return "OP_TRUE";
        case OP_FALSE: return "OP_FALSE";
        case OP_GREATER: return "OP_GREATER";
        case OP_LESS: return "OP_LESS";
        case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OP_JUMP_IF_TRUE: return "OP_JUMP_IF_TRUE";
        case OP_JUMP: return "OP_JUMP";
        case OP_JUMP_BACK: return "OP_JUMP_BACK";
        case OP_CALL: return "OP_CALL";
        case OP_RETURN: return "OP_RETURN";
        default: return "Unrecognized op";
    }
}

static int32_t read_int(Chunk* chunk, int offset) {
    int32_t* ptr = (int32_t*)(&chunk->codes[offset]);
    return *ptr;
}

static double read_float(Chunk* chunk, int offset) {
    double* ptr = (double*)(&chunk->codes[offset]);
    return *ptr;
}

static ObjString* read_string(Chunk* chunk, int offset) {
    ObjString** ptr = (ObjString**)(&chunk->codes[offset]);
    return *ptr;
}

static ObjFunction* read_function(Chunk* chunk, int offset) {
    ObjFunction** ptr = (ObjFunction**)(&chunk->codes[offset]);
    return *ptr;
}


void disassemble_chunk(Chunk* chunk) {
    int i = 0;
    while (i < chunk->count) {
        OpCode op = chunk->codes[i++];
        printf("%04d    [ %s ] ", i - 1, op_to_string(op));
        switch(op) {
            case OP_INT: 
                printf("%d", read_int(chunk, i)); 
                i += sizeof(int32_t);
                break;
            case OP_FLOAT: 
                printf("%f", read_float(chunk, i)); 
                i += sizeof(double);
                break;
            case OP_STRING: {
                ObjString* obj = read_string(chunk, i);
                printf("%s", obj->chars); 
                i += sizeof(ObjString*);
                break;
            }
            case OP_FUN: {
                ObjFunction* obj = read_function(chunk, i);
                printf("<fun>"); 
                i += sizeof(ObjFunction*);
                break;
            }
            case OP_GET_VAR: {
                uint8_t slot = chunk->codes[i];
                i++;
                printf("[%d]", slot);
                break;
            }
            case OP_SET_VAR: {
                uint8_t slot = chunk->codes[i];
                i++;
                printf("[%d]", slot);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t dis = (uint16_t)chunk->codes[i];
                i += 2;
                printf("->[%d]", dis + i); //i currently points to low bit in 'dis'
                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint16_t dis = (uint16_t)chunk->codes[i];
                i += 2;
                printf("->[%d]", dis + i); //i points to low bit in 'dis'
                break;
            }
            case OP_JUMP: {
                uint16_t dis = (uint16_t)chunk->codes[i];
                i += 2;
                printf("->[%d]", dis + i);
                break;
            }
            case OP_JUMP_BACK: {
                uint16_t dis = (uint16_t)chunk->codes[i];
                i += 2;
                printf("->[%d]", i - dis);
                break;
            }
            case OP_CALL: {
                uint8_t slot = chunk->codes[i];
                i++;
                printf("[%d]", slot);
                break;
            }
        }
        printf("\n");
    }
}
