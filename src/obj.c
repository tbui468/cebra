#include <string.h>
#include "obj.h"
#include "memory.h"


void insert_object(struct Obj* ptr) {
    if (mm.objects == NULL) {
        mm.objects = ptr;
        return; 
    }

    ptr->next = mm.objects;
    mm.objects = ptr;
}


int free_object(struct Obj* obj) {
    int bytes_freed = 0;
    switch(obj->type) {
        case OBJ_STRING: {
            struct ObjString* obj_string = (struct ObjString*)obj;
            bytes_freed += FREE_ARRAY(obj_string->chars, char, obj_string->length + 1);
            bytes_freed += FREE(obj_string, struct ObjString);
            break;
        }
        case OBJ_FUNCTION:
            struct ObjFunction* obj_fun = (struct ObjFunction*)obj;
            bytes_freed += free_chunk(&obj_fun->chunk);
            bytes_freed += FREE(obj_fun, struct ObjFunction);
            break;
        case OBJ_CLASS:
            struct ObjClass* oc = (struct ObjClass*)obj;
            //NOTE: this only frees the table entries - the key and any heap allocated values will
            //be freed by the GC
            bytes_freed += free_table(&oc->props);
            bytes_freed += free_table(&oc->castable_types);
            bytes_freed += FREE(oc, struct ObjClass);
            break;
        case OBJ_INSTANCE:
            struct ObjInstance* oi = (struct ObjInstance*)obj;
            bytes_freed += free_table(&oi->props);
            bytes_freed += FREE(oi, struct ObjInstance);
            break;
        case OBJ_ENUM:
            struct ObjEnum* oe = (struct ObjEnum*)obj;
            bytes_freed += free_table(&oe->props);
            bytes_freed += FREE(oe, struct ObjEnum);
            break;
        case OBJ_UPVALUE:
            struct ObjUpvalue* uv = (struct ObjUpvalue*)obj;
            bytes_freed += FREE(uv, struct ObjUpvalue);
            break;
        case OBJ_NATIVE:
            struct ObjNative* nat = (struct ObjNative*)obj;
            bytes_freed += FREE(nat, struct ObjNative);
            break;
        case OBJ_LIST:
            struct ObjList* list = (struct ObjList*)obj;
            bytes_freed += free_value_array(&list->values);
            bytes_freed += FREE(list, struct ObjList);
            break;
        case OBJ_MAP:
            struct ObjMap* map = (struct ObjMap*)obj;
            bytes_freed += free_table(&map->table);
            bytes_freed += FREE(map, struct ObjMap);
            break;
    }
    return bytes_freed;
}


void print_object(struct Obj* obj) {
    switch(obj->type) {
        case OBJ_STRING:
            printf("OBJ_STRING [");
            print_value(to_string((struct ObjString*)obj));
            printf("] : ");
            break;
        case OBJ_FUNCTION:
            printf("OBJ_FUNCTION [");
            print_value(to_function((struct ObjFunction*)obj));
            printf("] : ");
            break;
        case OBJ_CLASS:
            printf("OBJ_CLASS: ");
            break;
        case OBJ_INSTANCE:
            printf("OBJ_INSTANCE: ");
            break;
        case OBJ_ENUM:
            printf("OBJ_ENUM: ");
            break;
        case OBJ_UPVALUE:
            printf("OBJ_UPVALUE: ");
            print_value(((struct ObjUpvalue*)obj)->closed);
            printf(" ");
            break;
        case OBJ_NATIVE:
            printf("OBJ_NATIVE [");
            print_value(to_native((struct ObjNative*)obj));
            printf("] : ");
            break;
        case OBJ_LIST:
            printf("OBJ_LIST:");
            break;
        case OBJ_MAP:
            printf("OBJ_MAP:");
            break;
        default:
            printf("Invalid Object: ");
            break;
    }
    if (obj->is_marked) {
        printf("is marked, ");
    } else {
        printf("NOT marked, ");
    }
}

void mark_object(struct Obj* obj) {
    if (obj == NULL) return;
    obj->is_marked = true;
}


//struct ObjClass* make_class(struct ObjString* name, struct Table castable_types) {
struct ObjClass* make_class(Token name) {
    struct ObjString* struct_string = make_string(name.start, name.length);
    push_root(to_string(struct_string));
    struct ObjClass* obj = ALLOCATE(struct ObjClass);
    push_root(to_class(obj));
    obj->base.type = OBJ_CLASS;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->name = struct_string;
    init_table(&obj->props);
    init_table(&obj->castable_types);

    pop_root();
    pop_root();
    return obj;
}

struct ObjInstance* make_instance(struct Table table, struct ObjClass* klass) {
    struct ObjInstance* obj = ALLOCATE(struct ObjInstance);
    obj->base.type = OBJ_INSTANCE;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);
    obj->props = table;
    obj->klass = klass;

    return obj;
}

struct ObjEnum* make_enum(Token name) {
    struct ObjString* enum_string = make_string(name.start, name.length);
    push_root(to_string(enum_string));
    struct ObjEnum* obj = ALLOCATE(struct ObjEnum);
    push_root(to_enum(obj));
    obj->base.type = OBJ_ENUM;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);
    
    obj->name = enum_string;
    init_table(&obj->props);

    pop_root();
    pop_root();
    return obj;
}

struct ObjFunction* make_function(struct ObjString* name, int arity) {
    struct ObjFunction* obj = ALLOCATE(struct ObjFunction);
    push_root(to_function(obj));
    obj->base.type = OBJ_FUNCTION;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->name = name;
    obj->arity = arity;
    init_chunk(&obj->chunk);
    obj->upvalue_count = 0;

    pop_root();
    return obj;
}

struct ObjUpvalue* make_upvalue(Value* location) {
    struct ObjUpvalue* obj = ALLOCATE(struct ObjUpvalue);
    obj->base.type = OBJ_UPVALUE;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->location = location;
    obj->closed = to_nil();
    obj->next = NULL;

    return obj;
}

struct ObjNative* make_native(Value (*function)(int, Value*)) {
    struct ObjNative* obj = ALLOCATE(struct ObjNative);
    push_root(to_native(obj));
    obj->base.type = OBJ_NATIVE;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->function = function;

    pop_root();
    return obj;
}

struct ObjList* make_list() {
    struct ObjList* obj = ALLOCATE(struct ObjList);
    push_root(to_list(obj));
    obj->base.type = OBJ_LIST;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    obj->default_value = to_nil();
    insert_object((struct Obj*)obj);

    init_value_array(&obj->values);

    pop_root();
    return obj;
}

struct ObjMap* make_map() {
    struct ObjMap* obj = ALLOCATE(struct ObjMap);
    push_root(to_map(obj)); 
    obj->base.type = OBJ_MAP;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    obj->default_value = to_nil();
    insert_object((struct Obj*)obj);

    init_table(&obj->table);

    pop_root();
    return obj;
}

static uint32_t hash_string(const char* str, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619;
    }
    return hash;
}

struct ObjString* take_string(char* chars, int length) {
    struct ObjString* obj = ALLOCATE(struct ObjString);

    obj->base.type = OBJ_STRING;
    obj->base.next = NULL;
    obj->base.is_marked = false;

    obj->chars = chars;
    obj->length = length;
    obj->hash = hash_string(chars, length);

    insert_object((struct Obj*)obj);
    return obj;
}

struct ObjString* make_string(const char* start, int length) {
    char* chars = ALLOCATE_ARRAY(char);
    chars = GROW_ARRAY(chars, char, length + 1, 0);
    memcpy(chars, start, length);
    chars[length] = '\0';

    return take_string(chars, length);
}
