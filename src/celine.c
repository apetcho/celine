#include<string.h>
#include "celine.h"

#define CLN_FTABLE_INITIAL_CAPACITY     20
#define CLN_BUFLEN                      256

// -*-----------------------------------------------------------------*-
// -*- Type -> (IDTable)                                             -*-
// -*-----------------------------------------------------------------*-
// -
void cln_checktype(Object *obj, enum Type type){
    if(obj->type != type){
        cln_panic("TypeError: expected %d, got %d\n", type, obj->type);
    }
}

// -*-
char* cln_toString(const Object *self){
    char *buffer;
    int rc;
    buffer = cln_alloc(sizeof(char)*CLN_BUFLEN);
    switch(self->type){
    case TY_INTEGER:
        rc = sprintf(buffer, "%ld", self->val.integer);
        if(rc < 0){
            cln_panic("ValueError: unable to convert %ld to a string\n", self->val.integer);
        }
        break;
    case TY_FLOAT:
        rc = sprintf(buffer, "%lg", self->val.real);
        if(rc < 0){
            cln_panic("ValueError: unable to convert %lg to a string\n", self->val.real);
        }
        break;
    case TY_STRING:
        if(strlen(self->val.cstr) >= CLN_BUFLEN){
            cln_panic(
                "Too long string to output: '%s', maximum buffer size is %d\n",
                self->val.cstr, CLN_BUFLEN
            );
        }
        strcpy(buffer, self->val.cstr);
        break;
    case TY_ARRAY:
        strcpy(buffer, "[array]");
        break;
    case TY_FUN:
        strcpy(buffer, "[function]");
        break;
    case TY_OBJECT:     // [EXTENSION]
        strcpy(buffer, "[object]");
        break;
    default:
        cln_panic("Invalid data type: %d\n", self->type);
        break;
    }

    return buffer;
}

// -*-
Object* cln_new(){
    Object *self = cln_alloc(sizeof(Object));
    self->fields = (Field**)cln_alloc(sizeof(Field*)*CLN_FTABLE_INITIAL_CAPACITY);
    self->ftcap = CLN_FTABLE_INITIAL_CAPACITY;
    self->nfield = 0;
    memset(self->fields, 0, sizeof(Field*)*CLN_FTABLE_INITIAL_CAPACITY);
    return self;
}

// -*-
Object* cln_new_integer(long num){
    Object *self = cln_new();
    self->type = TY_INTEGER;
    self->val.integer = num;
    return self;
}

// -*-
Object* cln_new_float(double num){
    Object *self = cln_new();
    self->type = TY_FLOAT;
    self->val.real = num;
    return self;
}

// -*-
Object* cln_new_string(char *cstr){
    Object *self = cln_new();
    self->type = TY_STRING;
    self->val.cstr = cstr;
    return self;
}

Object* cln_new_builtin(int *args, int narg, Ast *code);
Object* cln_new_array(size_t len);
uint32_t cln_hash(const char* cstr, size_t tableLen);
void cln_set_field(Object *self, const char* name, Object *obj);
Object* cln_get_field(Object *self, const char* name);
Object* cln_get_field_generic(Object *self, const char* name, bool checkproto);