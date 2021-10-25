#include <stdio.h>
#include "memory.h"
#include "obj.h"


MemoryManager mm;

void push_gray(struct Obj* object) {
    if (object == NULL) return;
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

void push_root(Value value) {
//    printf("pushing root ");
//    print_value(value);
//    printf("stack size: %d\n", (int)(mm.vm->stack_top - mm.vm->stack));
    push(mm.vm, value);
}

Value pop_root() {
    return pop(mm.vm);
}

void* realloc_mem(void* ptr, size_t new_size, size_t old_size) {
    mm.allocated += (new_size - old_size);

#ifdef DEBUG_STRESS_GC
    collect_garbage();
#else
    if (mm.allocated > mm.next_gc) {
        collect_garbage();
        mm.next_gc = mm.allocated * 2;
    }
#endif

    void* result = realloc(ptr, new_size);
    if (result == NULL && new_size != 0) {
        printf("[Error] Memory allocation failed. Attempting to allocate %zu bytes\n", new_size);
        exit(1);
    }
    return result;
}

int free_mem(void* ptr, size_t size) {
    mm.allocated -= size;
    free(ptr);
    return size;
}

void init_memory_manager() {
    mm.allocated = 0;
    mm.next_gc = 1024 * 1024;
    mm.objects = NULL;
    //NOTE: using system realloc since we don't want GC to collect within a GC collection
    mm.grays = (struct Obj**)realloc(NULL, 0);
    mm.gray_capacity = 0;
    mm.gray_count = 0;
    mm.vm = NULL;
}


void free_memory_manager() {
    free((void*)mm.grays);
}

void print_memory() {
    printf("bytes allocated: %d\n", mm.allocated);
}

static void mark_and_push(struct Obj* obj) {
    if (obj != NULL && !obj->is_marked) {
        mark_object(obj);
        push_gray(obj);
    }
}

static void mark_table(struct Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        struct Pair* pair = &table->pairs[i];
        if (pair->key == NULL) continue;
        mark_and_push((struct Obj*)(pair->key));
        struct Obj* obj = get_object(&pair->value);
        mark_and_push(obj);
    }
}

static void mark_vm_roots() {
    for (Value* slot = mm.vm->stack; slot < mm.vm->stack_top; slot++) {
        struct Obj* obj = get_object(slot);
        mark_and_push(obj);
    }

    struct ObjUpvalue* current = mm.vm->open_upvalues;
    int count = 0;
    while (current != NULL) {
        count++;
        mark_and_push((struct Obj*)current);
        current = current->next;
    }


    if (mm.vm->initialized) {
        mark_table(&mm.vm->globals);
        mark_table(&mm.vm->strings);
    }
}

static void mark_compiler_roots() {
    struct Compiler* current = current_compiler;
    while (current != NULL) {
        mark_table(&current->globals);
        mark_and_push((struct Obj*)(current->function));

        struct Type* type = current->types;
        while (type != NULL) {
            switch(type->type) {
                case TYPE_ENUM: {
                    struct TypeEnum* te = (struct TypeEnum*)type;
                    mark_table(&te->props);
                    break;
                }
                case TYPE_STRUCT: {
                    struct TypeStruct* sc = (struct TypeStruct*)type;
                    mark_table(&sc->props);
                    break;
                }
                default:
                    break;
            }
            type = type->next;
        }

        current = current->enclosing;
    }

}

static void trace_references() {
    while (mm.gray_count > 0) {
        struct Obj* obj = pop_gray();
        switch(obj->type) {
            case OBJ_FUNCTION: {
                struct ObjFunction* fun = (struct ObjFunction*)obj;
                //upvalues
                for (int i = 0; i < fun->upvalue_count; i++) {
                    struct ObjUpvalue* uv = fun->upvalues[i];
                    struct Obj* uv_obj = (struct Obj*)uv;
                    mark_and_push(uv_obj);
                }
                //constants in chunk
                for (int i = 0; i < fun->chunk.constants.count; i++) {
                    Value* value = &fun->chunk.constants.values[i];
                    struct Obj* val_obj = get_object(value);
                    if (val_obj != NULL) {
                        mark_and_push(val_obj);
                    }
                }
                //ObjString* name
                mark_and_push((struct Obj*)(fun->name));
                break;
            }
            case OBJ_NATIVE: {
                struct ObjNative* on = (struct ObjNative*)obj;
                mark_and_push((struct Obj*)(on->name));
                break;
            }
            case OBJ_STRUCT: {
                struct ObjStruct* oc = (struct ObjStruct*)obj;
                //table of props and types
                mark_table(&oc->props);
                //class name
                mark_and_push((struct Obj*)(oc->name));
                //super
                mark_and_push((struct Obj*)(oc->super));
                break;
            }
            case OBJ_INSTANCE: {
                struct ObjInstance* oi = (struct ObjInstance*)obj;
                //table of props
                mark_table(&oi->props);
                //class
                mark_and_push((struct Obj*)(oi->klass));
                break;
            }
            case OBJ_ENUM: {
                struct ObjEnum* oe = (struct ObjEnum*)obj;
                mark_and_push((struct Obj*)(oe->name));

                mark_table(&oe->props);
                break;
            }
            case OBJ_UPVALUE: {
                struct ObjUpvalue* uv = (struct ObjUpvalue*)obj;
                //closed value
                struct Obj* closed_obj = get_object(&uv->closed);
                mark_and_push(closed_obj);
                break;
            }
            case OBJ_STRING: {
                break;
            }
            case OBJ_LIST: {
                struct ObjList* list = (struct ObjList*)obj;
                //values
                for (int i = 0; i < list->values.count; i++) {
                    Value* value = &list->values.values[i];
                    struct Obj* val_obj = get_object(value);
                    mark_and_push(val_obj);
                }
                //mark default_value
                struct Obj* val_obj = get_object(&list->default_value);
                mark_and_push(val_obj);
                break;
            }
            case OBJ_MAP: {
                struct ObjMap* om = (struct ObjMap*)obj;
                //table
                mark_table(&om->table);
                //default value
                struct Obj* val_obj = get_object(&om->default_value);
                mark_and_push(val_obj);
                break;
            }
            case OBJ_FILE: {
                struct ObjFile* of = (struct ObjFile*)obj;
                mark_and_push((struct Obj*)(of->file_path));
                mark_and_push((struct Obj*)(of->next_line));
            }
            default: {
            }
        }
    }
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
                mm.objects = current;
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

static void print_objects() {
    int count = 0;
    struct Obj* current = mm.objects;
    while (current != NULL) {
        count++;
        print_object(current);
        current = current->next;
    }
    printf("Object count: %d\n", count);
}

void collect_garbage() { 
#ifdef DEBUG_LOG_GC
    printf("- Start GC\n");
    printf("Bytes allocated: %d\n", mm.allocated);
    print_objects();
    print_stack(mm.vm); printf("\n");
#endif 
    mark_vm_roots();
    mark_compiler_roots();
    trace_references();
#ifdef DEBUG_LOG_GC
    print_objects();
#endif 
    int bytes_freed = sweep();
#ifdef DEBUG_LOG_GC
    printf("Bytes freed: %d\n", bytes_freed);
    printf("- End GC\n\n");
#endif
}
