#include<assert.h>
#include<errno.h>
#include<string.h>
#include "celine.h"

#define CLN_EVALOP(op)                                      \
    lhs = _cln_eval_expr(ast->node, env, symtable);         \
    cln_checktype(lhs, TY_INTEGER);                         \
    rhs = _cln_eval_expr(ast->node->next, env, symtable);   \
    cln_checktype(rhs, TY_INTEGER);                         \
    return cln_new_integer((long)((lhs->val.integer) op (rhs->val.integer)))

// -*---------------------------------------------------------------*-
// -*- Parser                                                      -*-
// -*---------------------------------------------------------------*-
// -*- Object* _cln_eval_expr() -*-
static Object* _cln_eval_expr(Ast *ast, Env *env, Symtable *symtable);

// -*- void _cln_narg_error()
static void _cln_narg_error(Object *fun){
    cln_panic(
        "CelineError: invalid number of arguments passed to function: expected %d\n",
        fun->val.fun.narg
    );
}

// -*- Object* _cln_eval_call()
static Object* _cln_eval_call(Env* env, Object *fun, Ast* arglist, Object *owner, Symtable *symtable){
    int narg = 0;
    cln_checktype(fun, TY_FUN);
    Env *local = cln_new_env();
    local->parent = env;
    Object **args = (Object**)cln_alloc(sizeof(Object*)*fun->val.fun.narg);
    for(Ast *arg = arglist->next; arg; arg = arg->next){
        if(narg==fun->val.fun.narg){
            _cln_narg_error(fun);
        }
        args[narg++] = _cln_eval_expr(arg, env, symtable);
    }
    if(narg < fun->val.fun.narg){
        _cln_narg_error(fun);
    }
    for(narg=0; narg < fun->val.fun.narg; ++narg){
        cln_env_put(local, fun->val.fun.args[narg], args[narg]);
    }
    cln_env_put(local, CLN_RETURN_ID, NULL);
    cln_env_put(local, CLN_SELF_ID, owner);
    cln_eval(fun->val.fun.code, local, symtable);
    Object *result = local->idents[CLN_RETURN_ID];
    cln_dealloc(args);
    cln_dealloc(local);
    return result;
}

// -*- Object** _cln_resolve_index()
static Object** _cln_resolve_index(Ast *ast, Env *env, Symtable *symtable){
    int i = ast->obj->val.integer;
    Object *index = _cln_eval_expr(ast->node, env, symtable);
    cln_checktype(index, TY_INTEGER);
    Object* self = cln_env_get(env, i);
    cln_checktype(self, TY_ARRAY);
    if(index->val.integer >= self->val.array.len){
        cln_panic(
            "Array index out of bounds: %d out of %d\n",
            index->val.integer, self->val.array.len
        );
    }

    return &self->val.array.data[index->val.integer];
}

// -*- void _cln_eval_set_field()
static void _cln_eval_set_field(Ast *ast, Env *env, Object *obj){
    int i = ast->obj->val.integer;
    Object *self = cln_env_get(env, i);
    cln_checktype(ast->node->obj, TY_STRING);
    cln_set_field(self, ast->node->obj->val.cstr, obj);
}

// -*- Object* _cln_eval_get_field()
static Object* _cln_eval_get_field(Ast *ast, Env *env){
    int i = ast->obj->val.integer;
    Object *self = cln_env_get(env, i);
    cln_checktype(ast->node->obj, TY_STRING);
    return cln_get_field(self, ast->node->obj->val.cstr);
}

// -*- Object* _cln_eval_def()
static Object* _cln_eval_def(Ast *ast){
    int* fargs = (int*)cln_alloc(sizeof(int)*CLN_BUILTIN_MAXARGS);
    int argc = 0;
    for(Ast* arg=ast->node; arg; arg = arg->next){
        fargs[argc++] = arg->obj->val.integer;
    }

    assert(ast->node);
    return cln_new_fun(fargs, argc, ast->node->next);
}

// -*-
static void _cln_readlong(long *num){
    char buf[64] = {0};
    char *rv = fgets(buf, sizeof(buf), stdin);
    if(rv == NULL){
        cln_panic("CelineError: error reading number from standard input\n");
    }
    char *ptr;
    *num = strtol(buf, &ptr, 10);
    if(errno == EINVAL || errno==ERANGE){
        cln_panic("CelineError: error reading number from standard input\n");
    }
    return;
}

// -*-
static void _cln_readfloat(double *num){
    char buf[64] = {0};
    char *rv = fgets(buf, sizeof(buf), stdin);
    if(rv == NULL){
        cln_panic("CelineError: error reading number from standard input\n");
    }
    char *ptr;
    *num = strtod(buf, &ptr);
    if(errno==ERANGE){
        cln_panic("CelineError: error reading number from standard input\n");
    }
    return;
}

// -*-
static void _cln_readstring(char **str){
    char buf[256];
    memset(buf, '\0', sizeof(buf));
    char *rv = fgets(buf, sizeof(buf), stdin);
    if(rv == NULL || errno==EBADF){
        cln_panic("CelineError: error reading number from standard input\n");
    }
    *str = strdup(buf);
    return;
}


// -*- Object* _cln_eval_expr()
static Object* _cln_eval_expr(Ast *ast, Env *env, Symtable *symtable){
    Object *lhs;
    Object *rhs;
    Object *self;
    Object *index;
    Object *len;
    long inum;
    double fnum;
    int idx;
    char *str = NULL;
    switch(ast->akind){
    case AST_IDENT:
        return cln_env_get(env, ast->obj->val.integer);
    case AST_READ_INT:
        printf("cln>> ");
        _cln_readlong(&inum);
        return cln_new_integer(inum);
    case AST_INPUT:
        printf("cln>> ");
        _cln_readstring(&str);
        return cln_new_string(str);
    case AST_ARRAY:
        len = _cln_eval_expr(ast->node, env, symtable);
        cln_checktype(len, TY_INTEGER);
        return cln_new_array(len->val.integer);
    case AST_OBJECT:
        return cln_new();
    case AST_INDEX:
        return *_cln_resolve_index(ast, env, symtable);
    case AST_FIELD:
        self = _cln_eval_get_field(ast, env);
        if(!self){
            cln_panic("CelineError: unknown field: %s\n", ast->node->obj->val.cstr);
        }
        return self;
    case AST_DEF:
        return _cln_eval_def(ast);
    case AST_CALL:
        return _cln_eval_call(
            env, cln_env_get(env, ast->obj->val.integer),
            ast->node, NULL, symtable
        );
    case AST_NEW:{
            Object *obj = cln_new();
            Object *ctor = cln_env_get(env, ast->node->obj->val.integer);
            _cln_eval_call(env, ctor, ast->node->node, obj, symtable);
            cln_set_field(obj, CLN_PROTOTYPE, cln_get_field_generic(ctor, CLN_PROTOTYPE, false));
            return obj;
        }//
    case AST_MCALL:
        return _cln_eval_call(
            env, _cln_eval_get_field(ast, env),
            ast->node->next, cln_env_get(env, ast->node->obj->val.integer),
            symtable
        );
    case AST_ADD:
        CLN_EVALOP(+);
    case AST_SUB:
        CLN_EVALOP(-);
    case AST_MUL:
        CLN_EVALOP(*);
    case AST_DIV:
        CLN_EVALOP(/);
    case AST_AND:
        CLN_EVALOP(&&);
    case AST_OR:
        CLN_EVALOP(||);
    case AST_NOT:
        self = _cln_eval_expr(ast->node, env, symtable);
        cln_checktype(self, TY_INTEGER);
        return cln_new_integer((long)(!self->val.integer));
    case AST_LT:
        CLN_EVALOP(<);
    case AST_EQ:
        CLN_EVALOP(==);
    case AST_GT:
        CLN_EVALOP(>);
    case AST_LE:
        CLN_EVALOP(<=);
    case AST_GE:
        CLN_EVALOP(>=);
    default:
        fprintf(
            stderr, "CelineError: unexpected syntax error: %s\n",
            clnAstKindNames[ast->akind]
        );
        return NULL;
    }
}

// -*- void _cln_eval_assign()
static void _cln_eval_assign(Ast *ast, Env *env, int local, Symtable *symtable);

// -*- void _cln_eval_statement()
static void _cln_eval_statement(Ast *ast, Env *env, Symtable *symtable);

// -*-
void cln_eval(Ast *ast, Env *env, Symtable *symtable);
