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

    return realloc(ptr, new_size);
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
    mm.vm_globals_initialized = false;
}


void free_memory_manager() {
    free((void*)mm.grays);
}

void print_memory() {
    printf("bytes allocated: %d\n", mm.allocated);
}

static void mark_table(struct Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        struct Pair* pair = &table->pairs[i];
        if (pair->key != NULL) {
            mark_object((struct Obj*)(pair->key));
            push_gray((struct Obj*)(pair->key));
            struct Obj* obj = get_object(&pair->value);
            mark_object(obj);
            push_gray(obj);
        }
    }
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
    int count = 0;
    while (current != NULL) {
        count++;
        mark_object((struct Obj*)current);
        push_gray((struct Obj*)current);
        current = current->next;
    }

    if (mm.vm_globals_initialized) {
        mark_table(&mm.vm->globals);
    }
}

static void mark_compiler_roots() {
    struct Compiler* current = current_compiler;
    while (current != NULL) {
        mark_table(&current->globals);

        mark_object((struct Obj*)(current->function));
        push_gray((struct Obj*)(current->function));

        struct Type* type = current->types;
        while (type != NULL) {
            switch(type->type) {
                case TYPE_ENUM:
                    struct TypeEnum* te = (struct TypeEnum*)type;
                    mark_table(&te->props);
                    break;
                case TYPE_CLASS:
                    struct TypeClass* sc = (struct TypeClass*)type;
                    mark_table(&sc->props);
                    break;
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
                //ObjString* name
                mark_object((struct Obj*)(fun->name));
                push_gray((struct Obj*)(fun->name));
                break;
            }
            case OBJ_CLASS: {
                struct ObjClass* oc = (struct ObjClass*)obj;
                //table of props and types
                mark_table(&oc->props);
                mark_table(&oc->castable_types);
                //class name
                mark_object((struct Obj*)(oc->name));
                push_gray((struct Obj*)(oc->name));
                break;
            }
            case OBJ_INSTANCE: {
                struct ObjInstance* oi = (struct ObjInstance*)obj;
                //table of props
                mark_table(&oi->props);
                //class
                mark_object((struct Obj*)(oi->klass));
                push_gray((struct Obj*)(oi->klass));
                break;
            }
            case OBJ_ENUM: {
                struct ObjEnum* oe = (struct ObjEnum*)obj;
                mark_object((struct Obj*)oe->name);
                push_gray((struct Obj*)(oe->name));

                mark_table(&oe->props);
                break;
            }
            case OBJ_UPVALUE: {
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
                break;
            }
            case OBJ_LIST: {
                struct ObjList* list = (struct ObjList*)obj;
                //values
                for (int i = 0; i < list->values.count; i++) {
                    Value* value = &list->values.values[i];
                    struct Obj* val_obj = get_object(value);
                    if (val_obj != NULL && !val_obj->is_marked) {
                        mark_object(val_obj);
                        push_gray(val_obj);
                    }
                }
                //mark default_value
                struct Obj* val_obj = get_object(&list->default_value);
                if (val_obj != NULL && !val_obj->is_marked) {
                    mark_object(val_obj);
                    push_gray(val_obj);
                }
                break;
            }
            case OBJ_MAP: {
                struct ObjMap* om = (struct ObjMap*)obj;
                //table
                mark_table(&om->table);
                //default value
                struct Obj* val_obj = get_object(&om->default_value);
                if (val_obj != NULL && !val_obj->is_marked) {
                    mark_object(val_obj);
                    push_gray(val_obj);
                }
                break;
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
