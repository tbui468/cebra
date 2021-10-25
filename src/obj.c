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
        case OBJ_FUNCTION: {
            struct ObjFunction* obj_fun = (struct ObjFunction*)obj;
            bytes_freed += free_chunk(&obj_fun->chunk);
            bytes_freed += FREE(obj_fun, struct ObjFunction);
            break;
        }
        case OBJ_STRUCT: {
            struct ObjStruct* oc = (struct ObjStruct*)obj;
            //NOTE: this only frees the table entries - the key and any heap allocated values will
            //be freed by the GC
            bytes_freed += free_table(&oc->props);
            bytes_freed += FREE(oc, struct ObjStruct);
            break;
        }
        case OBJ_FILE: {
            struct ObjFile* file = (struct ObjFile*)obj;
            if (file->fp != NULL)
                fclose(file->fp);
            file->fp = NULL;
            bytes_freed += FREE(file, struct ObjFile);
            break;
        }
        case OBJ_INSTANCE: {
            struct ObjInstance* oi = (struct ObjInstance*)obj;
            bytes_freed += free_table(&oi->props);
            bytes_freed += FREE(oi, struct ObjInstance);
            break;
        }
        case OBJ_ENUM: {
            struct ObjEnum* oe = (struct ObjEnum*)obj;
            bytes_freed += free_table(&oe->props);
            bytes_freed += FREE(oe, struct ObjEnum);
            break;
        }
        case OBJ_UPVALUE: {
            struct ObjUpvalue* uv = (struct ObjUpvalue*)obj;
            bytes_freed += FREE(uv, struct ObjUpvalue);
            break;
        }
        case OBJ_NATIVE: {
            struct ObjNative* nat = (struct ObjNative*)obj;
            bytes_freed += FREE(nat, struct ObjNative);
            break;
        }
        case OBJ_LIST: {
            struct ObjList* list = (struct ObjList*)obj;
            bytes_freed += free_value_array(&list->values);
            bytes_freed += FREE(list, struct ObjList);
            break;
        }
        case OBJ_MAP: {
            struct ObjMap* map = (struct ObjMap*)obj;
            bytes_freed += free_table(&map->table);
            bytes_freed += FREE(map, struct ObjMap);
            break;
        }
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
        case OBJ_STRUCT: {
            struct ObjStruct* c = (struct ObjStruct*)obj;
            printf("OBJ_STRUCT: ");
            print_object((struct Obj*)(c->name));
            break;
        }
        case OBJ_FILE: {
            printf("OBJ_FILE");
            break;
        }
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


struct ObjFile* make_file(FILE* fp, struct ObjString* file_path) {
    struct ObjFile* obj = ALLOCATE(struct ObjFile);
    obj->fp = fp;
    obj->file_path = file_path;
    obj->next_line = NULL;
    obj->is_eof = false;

    obj->base.type = OBJ_FILE;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    return obj;
}

struct ObjStruct* make_struct(struct ObjString* name, struct ObjStruct* super) {
    struct ObjStruct* obj = ALLOCATE(struct ObjStruct);
    push_root(to_struct(obj));
    obj->super = NULL;
    obj->base.type = OBJ_STRUCT;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->name = name;
    obj->super = super;
    init_table(&obj->props);

    pop_root();
    return obj;
}

struct ObjInstance* make_instance(struct Table table, struct ObjStruct* klass) {
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
    obj->upvalue_count = 0;
    init_chunk(&obj->chunk);

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

struct ObjNative* make_native(struct ObjString* name, ResultCode (*function)(int, Value*, struct ValueArray*)) {
    struct ObjNative* obj = ALLOCATE(struct ObjNative);
    push_root(to_native(obj));
    obj->base.type = OBJ_NATIVE;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    insert_object((struct Obj*)obj);

    obj->function = function;
    obj->name = name;

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

/*
struct ObjList* copy_list(struct ObjList* l) {
    struct ObjList* obj = ALLOCATE(struct ObjList);
    push_root(to_list(obj));
    obj->base.type = OBJ_LIST;
    obj->base.next = NULL;
    obj->base.is_marked = false;
    obj->default_value = to_nil();
    insert_object((struct Obj*)obj);

    init_value_array(&obj->values);

    for (int i = 0; i < l->values.count; i++) {
        add_value(&obj->values, copy_value(&l->values.values[i]));
    }

    pop_root();
    return obj;
}*/

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
    struct ObjString* interned = find_interned_string(&mm.vm->strings, chars, length, hash_string(chars, length));
    if (interned != NULL) {
        FREE_ARRAY(chars, char, length + 1);
        return interned;
    }

    struct ObjString* obj = ALLOCATE(struct ObjString);

    obj->base.type = OBJ_STRING;
    obj->base.next = NULL;
    obj->base.is_marked = false;

    obj->chars = chars;
    obj->length = length;
    obj->hash = hash_string(chars, length);

    insert_object((struct Obj*)obj);

    push_root(to_string(obj));
    set_table(&mm.vm->strings, obj, to_nil());  //TODO uncomment this (along with the find_interned_strings checks) to intern strings
    pop_root();
    return obj;
}

struct ObjString* make_string(const char* start, int length) {
    struct ObjString* interned = find_interned_string(&mm.vm->strings, start, length, hash_string(start, length));
    if (interned != NULL) return interned;

    char* chars = ALLOCATE_ARRAY(char);
    chars = GROW_ARRAY(chars, char, length + 1, 0);
    memcpy(chars, start, length);
    chars[length] = '\0';

    return take_string(chars, length);
}
