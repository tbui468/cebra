#include <stdint.h>
#include <stdio.h>

#include "obj_function.h"
#include "obj_string.h"
#include "chunk.h"
#include "memory.h"

void init_chunk(Chunk* chunk) {
    chunk->codes = ALLOCATE_ARRAY(OpCode);
    chunk->count = 0;
    chunk->capacity = 0;
    init_value_array(&chunk->constants);
}

void free_chunk(Chunk* chunk) {
    free_value_array(&chunk->constants);
    FREE_ARRAY(chunk->codes, OpCode, chunk->capacity);
}

const char* op_to_string(OpCode op) {
    switch(op) {
        case OP_INT: return "OP_INT";
        case OP_FLOAT: return "OP_FLOAT";
        case OP_STRING: return "OP_STRING";
        case OP_PRINT: return "OP_PRINT";
        case OP_SET_VAR: return "OP_SET_VAR";
        case OP_GET_VAR: return "OP_GET_VAR";
        case OP_SET_DEF: return "OP_SET_DEF";
        case OP_GET_DEF: return "OP_GET_DEF";
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

static uint8_t read_byte(Chunk* chunk, int code_idx) {
    return chunk->codes[code_idx];
}

static uint16_t read_short(Chunk* chunk, int code_idx) {
    return (uint16_t)chunk->codes[code_idx];
}

void disassemble_chunk(Chunk* chunk) {
    int i = 0;
    while (i < chunk->count) {
        OpCode op = chunk->codes[i++];
        printf("%04d    [ %s ] ", i - 1, op_to_string(op));
        switch(op) {
            case OP_INT: {
                int idx = read_byte(chunk, i++);
                printf("%d", chunk->constants.values[idx].as.integer_type); 
                break;
            }
            case OP_FLOAT: {
                int idx = read_byte(chunk, i++);
                printf("%f", chunk->constants.values[idx].as.float_type); 
                break;
            }
            case OP_STRING: {
                int idx = read_byte(chunk, i++);
                printf("%s", chunk->constants.values[idx].as.string_type->chars); 
                break;
            }
            case OP_GET_VAR: {
                int slot = read_byte(chunk, i++);
                printf("[%d]", slot);
                break;
            }
            case OP_SET_VAR: {
                int slot = read_byte(chunk, i++);
                printf("[%d]", slot);
                break;
            }
            case OP_GET_DEF: {
                int slot = read_byte(chunk, i++);
                printf("[%d]", slot);
                break;
            }
            case OP_SET_DEF: {
                int slot = read_byte(chunk, i++);
                int slot2 = read_byte(chunk, i++);
                printf("[%d]", slot);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t dis = read_short(chunk, i);
                i += 2;
                printf("->[%d]", dis + i); //i currently points to low bit in 'dis'
                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint16_t dis = read_short(chunk, i);
                i += 2;
                printf("->[%d]", dis + i); //i points to low bit in 'dis'
                break;
            }
            case OP_JUMP: {
                uint16_t dis = read_short(chunk, i);
                i += 2;
                printf("->[%d]", dis + i);
                break;
            }
            case OP_JUMP_BACK: {
                uint16_t dis = read_short(chunk, i);
                i += 2;
                printf("->[%d]", i - dis);
                break;
            }
            case OP_CALL: {
                int slot = read_byte(chunk, i++);
                printf("[%d]", slot);
                break;
            }
        }
        printf("\n");
    }
}
