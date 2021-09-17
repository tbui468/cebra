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

int free_chunk(Chunk* chunk) {
    int bytes_freed = 0;
    bytes_freed += free_value_array(&chunk->constants);
    bytes_freed += FREE_ARRAY(chunk->codes, OpCode, chunk->capacity);
    return bytes_freed;
}

const char* op_to_string(OpCode op) {
    switch(op) {
        case OP_CONSTANT: return "OP_CONSTANT";
        case OP_FUN: return "OP_FUN";
        case OP_NIL: return "OP_NIL";
        case OP_PRINT: return "OP_PRINT";
        case OP_TRUE: return "OP_TRUE";
        case OP_FALSE: return "OP_FALSE";
        case OP_LESS: return "OP_LESS";
        case OP_GREATER: return "OP_GREATER";
        case OP_EQUAL: return "OP_EQUAL";
        case OP_GET_PROP: return "OP_GET_PROP";
        case OP_SET_PROP: return "OP_SET_PROP";
        case OP_SET_LOCAL: return "OP_SET_LOCAL";
        case OP_GET_LOCAL: return "OP_GET_LOCAL";
        case OP_SET_UPVALUE: return "OP_SET_UPVALUE";
        case OP_GET_UPVALUE: return "OP_GET_UPVALUE";
        case OP_CLOSE_UPVALUE: return "OP_CLOSE_UPVALUE";
        case OP_ADD: return "OP_ADD";
        case OP_SUBTRACT: return "OP_SUBTRACT";
        case OP_MULTIPLY: return "OP_MULTIPLY";
        case OP_DIVIDE: return "OP_DIVIDE";
        case OP_MOD: return "OP_MOD";
        case OP_NEGATE: return "OP_NEGATE";
        case OP_POP: return "OP_POP";
        case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OP_JUMP_IF_TRUE: return "OP_JUMP_IF_TRUE";
        case OP_JUMP: return "OP_JUMP";
        case OP_JUMP_BACK: return "OP_JUMP_BACK";
        case OP_CALL: return "OP_CALL";
        case OP_CLASS: return "OP_CLASS";
        case OP_ADD_PROP: return "OP_ADD_PROP";
        case OP_INSTANCE: return "OP_INSTANCE";
        case OP_RETURN: return "OP_RETURN";
        case OP_NATIVE: return "OP_NATIVE";
        default: return "Unrecognized op";
    }
}

static uint8_t read_byte(Chunk* chunk, int code_idx) {
    return chunk->codes[code_idx];
}

static uint16_t read_short(Chunk* chunk, int code_idx) {
    return (uint16_t)chunk->codes[code_idx];
}

void disassemble_chunk(struct ObjFunction* function) {
    printf("%.*s\n", function->name->length, function->name->chars);
    Chunk* chunk = &function->chunk;
    int i = 0;
    while (i < chunk->count) {
        OpCode op = chunk->codes[i++];
        printf("%04d    [ %s ] ", i - 1, op_to_string(op));
        switch(op) {
            case OP_CONSTANT: {
                int idx = read_short(chunk, i);
                i += 2;
                print_value(chunk->constants.values[idx]);
                break;
            }
            case OP_FUN: {
                int fun_idx = read_byte(chunk, i++);
                int upvalue_count = read_byte(chunk, i++);
                for (int i = 0; i < upvalue_count; i++) {
                    int uv_local = read_byte(chunk, i++);
                    int uv_idx = read_byte(chunk, i++);
                }
                printf("<fun>"); 
                break;
            }
            case OP_NIL: {
                printf("%s", op_to_string(op));
                break;
            }
            case OP_ADD_PROP: {
                int slot = read_byte(chunk, i++);
                printf("[%d]", slot);
                break;
            }
            case OP_GET_LOCAL: {
                int slot = read_byte(chunk, i++);
                printf("[%d]", slot);
                break;
            }
            case OP_SET_LOCAL: {
                int slot = read_byte(chunk, i++);
                printf("[%d]", slot);
                break;
            }
            case OP_GET_PROP: {
                int slot = read_byte(chunk, i++);
                printf("[%d]", slot);
                break;
            }
            case OP_SET_PROP: {
                int slot = read_byte(chunk, i++);
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
            case OP_NATIVE: {
                int slot = read_short(chunk, i);
                i += 2;
                printf("[%d]", slot);
                break;
            }
        }
        printf("\n");
    }
}
