
#include "celine.h"

#define CLN_FIELDS_TABLE_INITIAL_SIZE   20
#define CLN_BUFLEN                      256

// -*-----------------------------------------------------------------*-
// -*- Type -> (IDTable)                                             -*-
// -*-----------------------------------------------------------------*-
// -
void cln_checktype(Object *obj, enum Type type){
    
}


char* cln_toString(const Object *self);
Object* cln_new_integer(long num);
Object* cln_new_float(double num);
Object* cln_new_string(char *cstr);
Object* cln_new_builtin(int *args, int narg, Ast *code);
Object* cln_new_array(size_t len);
Object* cln_new();
uint32_t cln_hash(const char* cstr, size_t tableLen);
void cln_set_field(Object *self, const char* name, Object *obj);
Object* cln_get_field(Object *self, const char* name);
Object* cln_get_field_generic(Object *self, const char* name, bool checkproto);