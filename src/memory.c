#include <stdio.h>
#include "memory.h"
#include "obj_function.h"

MemoryManager mm;

void push_gray(struct Obj* object) {
    //NOTE: using system realloc since we don't want GC to collect within a GC collection
    if (mm.gray_count + 1 > mm.gray_capacity) {
        int new_capacity = mm.gray_capacity == 0 ? 8 : mm.gray_capacity * 2;
        mm.grays = (struct Obj**)realloc((void*)mm.grays, sizeof(struct Obj*) * new_capacity);
        if (mm.grays == NULL) exit(1);

        mm.gray_capacity = new_capacity;
    }

   mm.grays[mm.gray_count] = object;
   mm.gray_count++;
}

struct Obj* pop_gray() {
    mm.gray_count--;
    return mm.grays[mm.gray_count];
}

void* realloc_mem(void* ptr, size_t new_size, size_t old_size) {
    mm.bytes_allocated += (new_size - old_size);

#ifdef DEBUG_STRESS_GC
    collect_garbage();
#endif

    return realloc(ptr, new_size);
}

int free_mem(void* ptr, size_t size) {
    mm.bytes_freed += size;
    free(ptr);
    return size;
}

void init_memory_manager(VM* vm) {
    mm.bytes_allocated = 0;
    mm.bytes_freed = 0;
    mm.objects = NULL;
    mm.vm = vm;
    //NOTE: using system realloc since we don't want GC to collect within a GC collection
    mm.grays = (struct Obj**)realloc(NULL, 0);
    mm.gray_capacity = 0;
    mm.gray_count = 0;
}

void print_memory() {
    printf("bytes allocated: %d\n", mm.bytes_allocated);
    printf("bytes freed: %d\n", mm.bytes_freed);
}

//TODO: remove later - only to check that all memory is being tracked and cleared
void free_objects() {
    struct Obj* obj = mm.objects;
    while (obj != NULL) {
        struct Obj* current = obj;
        obj = obj->next;
        free_object(current);
    }
    mm.objects = NULL;
}

static void mark_vm_roots() {
    for (Value* slot = mm.vm->stack; slot < mm.vm->stack_top; slot++) {
        struct Obj* obj = get_object(slot);
        if (obj != NULL) {
            mark_object(obj);
            push_gray(obj);
        }
    }

    struct ObjUpvalue* current = mm.vm->open_upvalues;
    while (current != NULL) {
        mark_object((struct Obj*)current);
        push_gray((struct Obj*)current);
        current = current->next;
    }    
}

static void mark_compiler_roots() {
    struct Compiler* current = cc;
    while (current != NULL) {
        mark_object((struct Obj*)(current->function));
        push_gray((struct Obj*)(current->function));
        current = current->enclosing;
    }
}

static void trace_references() {
    printf("Starting grays: %d\n", mm.gray_count);
    while (mm.gray_count > 0) {
        printf("grays: %d\n", mm.gray_count);
        struct Obj* obj = pop_gray();
        printf("type: %d, ", obj->type);
        switch(obj->type) {
            case OBJ_FUNCTION: {
                printf("in function\n");
                struct ObjFunction* fun = (struct ObjFunction*)obj;
                //upvalues
                for (int i = 0; i < fun->upvalue_count; i++) {
                    struct ObjUpvalue* uv = fun->upvalues[i];
                    struct Obj* uv_obj = (struct Obj*)uv;
                    if (!uv_obj->is_marked) {
                        mark_object(uv_obj);
                        push_gray(uv_obj);
                    } 
                }
                //constants in chunk
                for (int i = 0; i < fun->chunk.constants.count; i++) {
                    Value* value = &fun->chunk.constants.values[i];
                    struct Obj* val_obj = get_object(value);
                    if (val_obj != NULL && !val_obj->is_marked) {
                        mark_object(val_obj);
                        push_gray(val_obj);
                    }
                }
                break;
            }
            case OBJ_UPVALUE: {
                printf("in upvalue\n");
                struct ObjUpvalue* uv = (struct ObjUpvalue*)obj;
                //closed value
                struct Obj* closed_obj = get_object(&uv->closed);
                if (closed_obj != NULL && !closed_obj->is_marked) {
                    mark_object(closed_obj);
                    push_gray(closed_obj);
                }
                break;
            }
            case OBJ_STRING: {
                printf("in string\n");
                break;
            }
            default: {
            }
        }
    }
    printf("\n");
}

static int sweep() {
    struct Obj* previous = NULL;
    struct Obj* current = mm.objects;
    int bytes_freed = 0;
    while (current != NULL) {
        if (current->is_marked) {
            current->is_marked = false;
            previous = current;
            current = current->next;
        } else {
            if (previous == NULL) {
                struct Obj* next = current->next;
                bytes_freed += free_object(current);
                current = next;
            } else {
                previous->next = current->next;
                bytes_freed += free_object(current);
                current = previous->next;
            }
        }
    }
    return bytes_freed;
}

static void print_marks() {
    struct Obj* current = mm.objects;
    while (current != NULL) {
        print_object(current);
        current = current->next;
    }
    printf("\n");
}

void collect_garbage() {
#ifdef DEBUG_LOG_GC
    printf("- Start GC\n");
    printf("\n");
    print_marks();
#endif 
    printf("marking vm roots\n");
    mark_vm_roots();
    print_marks();
    printf("marking compiler roots\n");
    mark_compiler_roots();
    print_marks();
    printf("tracing references\n");
    trace_references();
    print_marks();

    printf("Stack: ");
    Value* start = mm.vm->stack;
    while (start < mm.vm->stack_top) {
        printf("[ ");
        print_value(*start);
        printf(" ]");
        start++;
    }
    printf("\n");

    printf("sweeping\n");
    int bytes_freed = sweep();
#ifdef DEBUG_LOG_GC
    printf("Bytes freed: %d\n", bytes_freed);
    printf("- End GC\n\n");
#endif 
}
