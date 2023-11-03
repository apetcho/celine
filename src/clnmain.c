#include<assert.h>
#include<errno.h>
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
    char buf[64];
    fgets(buf, sizeof(buf), stdin);
    char *ptr;
    *num = strtol(buf, &ptr, 10);
    if(errno == EINVAL || errno==ERANGE){
        cln_panic("CelineError: error reading number from standard input\n");
    }
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
}

// -*- void _cln_eval_assign()
static void _cln_eval_assign(Ast *ast, Env *env, int local, Symtable *symtable);

// -*- void _cln_eval_statement()
static void _cln_eval_statement(Ast *ast, Env *env, Symtable *symtable);

// -*-
void cln_eval(Ast *ast, Env *env, Symtable *symtable);
