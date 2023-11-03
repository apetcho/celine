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

// -*-
Object* cln_new_fun(int *args, int narg, Ast *code){
    Object *self = cln_new();
    self->type = TY_FUN;
    self->val.fun.narg = narg;
    self->val.fun.args = args;
    self->val.fun.code = code;

    return self;
}

// -*-
Object* cln_new_array(size_t len){
    Object *self = cln_new();
    self->type = TY_ARRAY;
    self->val.array.len = len;
    self->val.array.data = (Object**)cln_alloc(sizeof(Object*)*len);
    cln_set_field(self, "len", cln_new_integer(len));
    // ... OTHER API ...
    return self;
}

// -*-
uint32_t cln_hash(const char* cstr, size_t tableLen){
    size_t len = strlen(cstr);
    uint32_t result = 0;
    for(int i=0; i < len; ++i){
        result = (result * 31 + cstr[i]) % tableLen;
    }

    return result;
}

// -*-
static uint32_t _cln_get_field_index(Object *self, const char* fieldname){
    uint32_t index = cln_hash(fieldname, self->ftcap);
    while(self->fields[index] && strcmp(self->fields[index]->name, fieldname)!=0){
        ++index;
        if(index >= self->ftcap){
            index = 0;
        }
    }
    return index;
}

// -
static void _cln_rehash(Object *self){
    size_t cap = self->ftcap;
    self->ftcap *= 2;
    Field **fields = self->fields;
    self->fields = (Field**)cln_alloc(sizeof(Field*)*self->ftcap);
    memset(self->fields, 0, sizeof(Field*)*self->ftcap);
    self->nfield = 0;
    uint32_t hash;
    Field *field;
    for(size_t i=0; i < cap; ++i){
        field = fields[i];
        if(field){
            cln_set_field(self, field->name, field->obj);
            cln_dealloc(field);
        }
    }
    cln_dealloc(fields);
}

// -*-
void cln_set_field(Object *self, const char* name, Object *obj){
    /* rehash if the load factor is greater than 0.75 */
    if(5*self->nfield > 3*self->ftcap){
        _cln_rehash(self);
    }
    uint32_t index = _cln_get_field_index(self, name);
    Field *field = (Field*)cln_alloc(sizeof(Field));
    char* fname = (char*)cln_alloc(sizeof(char)*(strlen(name)+1));
    strcpy(fname, name);
    field->name = fname;
    field->obj = obj;
    if(!self->fields[index]){
        ++self->nfield;
    }else if(!obj){
        --self->nfield;
    }else{
        cln_dealloc(self->fields[index]->name);
        cln_dealloc(self->fields[index]);
    }
    self->fields[index] = field;
}

// -*-
Object* cln_get_field_generic(Object *self, const char* name, bool checkproto){
    uint32_t index = _cln_get_field_index(self, name);
    if(self->fields[index]){
        return self->fields[index]->obj;
    }

    if(checkproto){
        Object *proto = cln_get_field_generic(self, CLN_PROTOTYPE, false);
        if(proto){
            return cln_get_field_generic(proto, name, checkproto);
        }
    }

    return NULL;
}

// -*-
Object* cln_get_field(Object *self, const char* name){
    return cln_get_field_generic(self, name, true);
}

// -*---------------------------------------------------------------*-
// -*- Ast                                                         -*-
// -*---------------------------------------------------------------*-

char* clnAstKindNames[] = {
#define CLN_DEF(kind, name)     name,
    CLN_AST_NODES
#undef CLN_DEF
};

// -*-
Ast* cln_new_ast(enum AstKind akind, Object *obj){
    Ast *ast = (Ast*)cln_alloc(sizeof(Ast));
    ast->akind = akind;
    ast->obj = obj;
    ast->node = NULL;
    ast->next = NULL;
    ast->parent = NULL;

    return ast;
}

// -*-
void cln_ast_add_node(Ast *parent, Ast *node){
    Ast *self = parent->node;
    self->parent = parent;
    if(!self){
        parent->node = node;
        return;
    }
    while(self->next){
        self = self->next;
    }
    self->next = node;
}

// -
static void _cln_dump_with_indent(Ast *ast, uint32_t indent){
    for(Ast *node = ast; node; node = node->next){
        for(uint32_t i=0; i < indent; ++i){
            printf(" ");
        }
        printf("%s", clnAstKindNames[node->akind]);
        if(node->akind==AST_IDENT || node->akind==AST_CONST){
            char* repr = cln_toString(node->obj);
            printf("(%s)", repr);
            cln_dealloc(repr);
        }
        printf("\n");
        if(node->node){
            _cln_dump_with_indent(node->node, indent+1);
        }
    }
}

// -*-
void cln_dump(Ast *ast){
    _cln_dump_with_indent(ast, 0);
}