#include<assert.h>

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
static void _cln_narg_error(Object *fun);

// -*- Object* _cln_eval_call()
static Object* _cln_eval_call(Env* env, Object *fun, Ast* arglist, Object *owner, Symtable *symtable);

// -*- Object** _cln_resolve_index()
static Object** _cln_resolve_index(Ast *ast, Env *env, Symtable *symtable);

// -*- void _cln_eval_set_field()
static void _cln_eval_set_field(Ast *ast, Env *env, Object *obj);

// -*- Object* _cln_eval_get_field()
static Object* _cln_eval_get_field(Ast *ast, Env *env);

// -*- Object* _cln_eval_def()
static Object* _cln_eval_def(Ast*);

// -*- Object* _cln_eval_expr()
static Object* _cln_eval_expr(Ast *ast, Env *env, Symtable *symtable);

// -*- void _cln_eval_assign()
static void _cln_eval_assign(Ast *ast, Env *env, int local, Symtable *symtable);

// -*- void _cln_eval_statement()
static void _cln_eval_statement(Ast *ast, Env *env, Symtable *symtable);

// -*-
void cln_eval(Ast *ast, Env *env, Symtable *symtable);
