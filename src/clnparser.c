#include<assert.h>
#include "celine.h"

typedef struct {
    const char *filename;
    Lexer *lexer;
    Token currentToken;
    Token nextToken;
} Parser;

// fail_with_unexpected_token()
static void _cln_fail_with_unexpected_token(Parser *parser, int got, int needed){
    fprintf(
        stderr,
        "CelineError: unexpected token at line %s:%d: \"%s\", needed \"%s\"\n",
        parser->filename, parser->currentToken.lineno,
        clnTokenNames[got], clnTokenNames[needed]
    );
    exit(EXIT_FAILURE);
}

// fail_with_parsing_error()
void _cln_fail_with_parsing_error(const char *message){
    cln_panic("CelineError: parsing error: %s\n", message);
}

// advance()
static void _cln_advance(Parser *parser){
    parser->currentToken = parser->nextToken;
    if(parser->currentToken.tkind != TOK_EOF){
        parser->nextToken = cln_lexer_nexttoken(parser->lexer);
    }
}

// match()
static Object* _cln_match(Parser *parser, int expectToken){
    if(parser->currentToken.tkind != expectToken){
        _cln_fail_with_unexpected_token(
            parser, parser->currentToken.tkind, expectToken
        );
    }
    Object *obj = parser->currentToken.obj;
    _cln_advance(parser);
    return obj;
}
// 

// -*-
static Ast* _cln_parse_program(Parser *parser);
static Ast* _cln_parse_oplist(Parser *parser);
static Ast* _cln_parse_block(Parser *parser);
static Ast* _cln_parse_condition(Parser *parser);
static Ast* _cln_parse_logical_expr(Parser *parser);
static Ast* _cln_parse_op(Parser *parser);

static Ast* _cln_parse_array_indexing(Parser *parser);
static Ast* _cln_parse_field(Parser *parser);

static Ast* _cln_parse_assign(Parser *parser);
static Ast* _cln_parse_while(Parser *parser);
static Ast* _cln_parse_if(Parser *parser);
static Ast* _cln_parse_call(Parser *parser);

static Ast* _cln_parse_arglist(Parser *parser);
static Ast* _cln_parse_expr(Parser *parser);
static Ast* _cln_parse_artith_expr(Parser *parser);
static Ast* _cln_parse_term(Parser *parser);
static Ast* _cln_parse_value(Parser *parser);
static Ast* _cln_parse_print(Parser *parser);
static Ast* _cln_parse_def(Parser *parser);
static Ast* _cln_parse_return(Parser *parser);
static Ast* _cln_parse_import(Parser *parser);
static Ast* _cln_parse_load(Parser *parser);

static Ast* _cln_parse_array(Parser *parser);
static Ast* _cln_parse_object(Parser *parser);


// -*-
Ast* cln_parse(const char* filename, Symtable *symtable){
    Parser parser;
    parser.filename = filename;
    parser.lexer = (Lexer*)cln_alloc(sizeof(Lexer));
    cln_lexer_init(parser.lexer, filename, symtable);
    Ast *ast = _cln_parse_program(&parser);
    printf("Symbol ID table: \n");
    for(int i=0; i < symtable->len; ++i){
        printf("%d = %s\n", i, symtable->symbols[i]);
    }
    cln_dealloc(parser.lexer);
    return ast;
}

// -*-
/*
expr | statement
expr | statement
....
*/
static Ast* _cln_parse_program(Parser *parser){
    // parser->nextToken = cln_lexer_nexttoken(parser->lexer);
    // _cln_advance(parser);
    return _cln_parse_oplist(parser);
}

// -*-
static Ast* _cln_parse_oplist(Parser *parser){
    Ast *ast = cln_new_ast(AST_EMPTY, CLN_NONE);
    while(parser->currentToken.tkind != TOK_RBRACE || parser->currentToken.tkind != TOK_EOF){ // XXX
        Ast *node = _cln_parse_op(parser);
        cln_ast_add_node(ast, node);
    }

    return ast;
}

// -*-
static Ast* _cln_parse_op(Parser *parser){
    Ast *ast = NULL;
    switch(parser->currentToken.tkind){
    case TOK_LBRACE: // { ...
        return _cln_parse_block(parser);
    case TOK_IDENT: // ident | ident(...)
        // assign, [eval], call
        ast = (
            parser->nextToken.tkind==TOK_LPAREN ?
            _cln_parse_call(parser) :
            _cln_parse_assign(parser)
        );
        _cln_match(parser, TOK_SEMI);
        return ast;
    case TOK_LOCAL: // local ...
        ast = _cln_parse_assign(parser);
        _cln_match(parser, TOK_SEMI);
        return ast;
    case TOK_WHILE:
        return _cln_parse_while(parser);
    case TOK_IF:
        return _cln_parse_if(parser);
    case TOK_DEF:
        return _cln_parse_def(parser);
    case TOK_RETURN: // return expr;
        ast = _cln_parse_return(parser);
        _cln_match(parser, TOK_SEMI);
        return ast;
    case TOK_PRINT: // print '(' ... ')'
        ast = _cln_parse_print(parser);
        _cln_match(parser, TOK_SEMI);
        return ast;
    case TOK_IMPORT: // import ...
        return _cln_parse_import(parser);
    case TOK_LOAD:
        ast = _cln_parse_load(parser);
        _cln_match(parser, TOK_SEMI);
        return ast;
    default:
        break;
    }
    _cln_fail_with_unexpected_token(parser, parser->currentToken.tkind, TOK_IDENT);
    return ast;
}

// -*-
static Ast* _cln_parse_block(Parser *parser){
    // { ... }
    _cln_match(parser, TOK_LBRACE);
    Ast *ast = _cln_parse_oplist(parser);
    _cln_match(parser, TOK_RBRACE);
    return ast;
}

// -*-
static Ast* _cln_parse_array_indexing(Parser *parser){
    Object *ident = _cln_match(parser, TOK_IDENT);
    _cln_match(parser, TOK_LSBRACKET); // [
    Ast *index = _cln_parse_expr(parser);
    _cln_match(parser, TOK_RSBRACKET);
    //! @todo : we should check that "index is integer object"
    Ast *ast = cln_new_ast(AST_INDEX, ident);
    cln_ast_add_node(ast, index);
    return ast;
}

// -*- ident.ident === ident.field 
static Ast* _cln_parse_field(Parser *parser){
    Object *ident = _cln_match(parser, TOK_IDENT);
    _cln_match(parser, TOK_DOT);
    Object *field = _cln_match(parser, TOK_FIELD);
    Ast *ast = cln_new_ast(AST_FIELD, ident);
    //! @note: ORIGINAL :: cln_ast_add_node(ast, cln_new_ast(AST_CONSTANT, field));
    //! @todo: cln_ast_add_node(ast, cln_new_ast(AST_SYMBOL, field));
    cln_ast_add_node(ast, cln_new_ast(AST_IDENT, field));
    return ast;
}

// -*-
static Ast* _cln_parse_assign(Parser *parser){
    Ast *lhs = NULL;
    enum AstKind akind = AST_ASSIGN;
    if(parser->currentToken.tkind == TOK_LOCAL){ // local
        _cln_match(parser, TOK_LOCAL);
        akind = AST_LOCAL;
    }

    if(parser->nextToken.tkind == TOK_LSBRACKET){ // ident[
        lhs = _cln_parse_array_indexing(parser);
    }else if(parser->nextToken.tkind == TOK_DOT){ // ident.
        lhs = _cln_parse_field(parser);
    }else{ // ident
        Object *ident = _cln_match(parser, TOK_IDENT);
        lhs = cln_new_ast(AST_IDENT, ident);
    }
    // lhs =
    _cln_match(parser, TOK_ASSIGN);
    // [local] (ident\.?\[?idx\]?) = expr
    Ast *expr = _cln_parse_expr(parser);
    Ast *ast = cln_new_ast(akind, CLN_NONE);
    cln_ast_add_node(ast, lhs);
    cln_ast_add_node(ast, expr);
    return ast;
}

// -*- print '(' expr ')'
static Ast* _cln_parse_print(Parser *parser){
    _cln_match(parser, TOK_PRINT);          // print
    _cln_match(parser, TOK_LPAREN);         // (
    Ast *expr = _cln_parse_expr(parser);    // expr
    _cln_match(parser, TOK_RPAREN);         // )
    Ast *ast = cln_new_ast(AST_PRINT, CLN_NONE);
    ast->node = expr;
    return ast;
}

// -*- def name(arglist){...}
static Ast* _cln_parse_def(Parser *parser){
    _cln_match(parser, TOK_DEF);
    Object *name = NULL;
    if(parser->currentToken.tkind == TOK_IDENT){
        name = _cln_match(parser, TOK_IDENT);
    }
    // name == NULL -> anonymous
    // name != NULL -> named function
    Ast *fun = cln_new_ast(AST_DEF, name);      // fname
    _cln_match(parser, TOK_LPAREN);             // (
    Ast *arglist = _cln_parse_arglist(parser);  // args
    _cln_match(parser, TOK_RPAREN);             // )
    Ast *body = _cln_parse_block(parser);       // { body }

    cln_ast_add_node(fun, arglist);
    cln_ast_add_node(fun, body);
    return fun;
}

// -*- while '(' cond ')' { body }
static Ast* _cln_parse_while(Parser *parser){
    _cln_match(parser, TOK_WHILE);
    Ast *ast = cln_new_ast(AST_WHILE, CLN_NONE);        // while
    _cln_match(parser, TOK_LPAREN);                     // (
    Ast *cond = _cln_parse_condition(parser);           // cond
    _cln_match(parser, TOK_RPAREN);                     // )
    //! @note: change from parse_op() to parse_block()
    Ast* body = _cln_parse_block(parser);               // { body }
    cln_ast_add_node(ast, cond);
    cln_ast_add_node(ast, body);

    return ast; 
}

// -*- not expr
static Ast* _cln_parse_condition(Parser *parser){
    if(parser->currentToken.tkind == TOK_NOT){
        Ast *cond = cln_new_ast(AST_NOT, CLN_NONE);
        _cln_match(parser, TOK_NOT);
        cln_ast_add_node(cond, _cln_parse_logical_expr(parser));
        return cond;
    }else{ // lhs op rhs
        Ast *lhs = _cln_parse_logical_expr(parser);
        enum TokenKind kind = parser->currentToken.tkind;
        Ast *rhs;
        if(kind == TOK_AND || kind == TOK_OR){
            _cln_match(parser, kind);
            rhs = _cln_parse_condition(parser);
            Ast *cond = cln_new_ast(
                kind == TOK_AND ? AST_AND : AST_OR, CLN_NONE
            );
            cln_ast_add_node(cond, lhs);
            cln_ast_add_node(cond, rhs);
            return cond;
        }
        return lhs;
    }
}

// -*- <, >, <=, >=, ==,
static Ast* _cln_parse_logical_expr(Parser *parser){
    Ast *val = _cln_parse_value(parser);
    Ast *ast = NULL;
    switch(parser->currentToken.tkind){
    case TOK_LT:
        ast = cln_new_ast(AST_LT, CLN_NONE);
        break;
    case TOK_EQ:
        ast = cln_new_ast(AST_EQ, CLN_NONE);
        break;
    case TOK_GT:
        ast = cln_new_ast(AST_GT, CLN_NONE);
        break;
    case TOK_LE:
        ast = cln_new_ast(AST_LE, CLN_NONE);
        break;
    case TOK_GE:
        ast = cln_new_ast(AST_GE, CLN_NONE);
        break;
    default:
        return val;
    }

    _cln_advance(parser);
    Ast *rhs = _cln_parse_value(parser);
    cln_ast_add_node(ast, val);
    cln_ast_add_node(ast, rhs);

    return ast;
}

// -*- if (cond) {}
static Ast* _cln_parse_if(Parser *parser){
    _cln_match(parser, TOK_IF);                     // if
    Ast *ast = cln_new_ast(AST_IF, CLN_NONE);
    _cln_match(parser, TOK_LPAREN);                 // (
    Ast *cond = _cln_parse_condition(parser);       // cond
    _cln_match(parser, TOK_RPAREN);                 // )
    Ast *body = _cln_parse_block(parser);           // { body }
    cln_ast_add_node(ast, cond);
    cln_ast_add_node(ast, body);
    // - else{ body}
    if(parser->currentToken.tkind == TOK_ELSE){
        _cln_match(parser, TOK_ELSE);
        Ast *alt = _cln_parse_block(parser);
        cln_ast_add_node(ast, alt);
    }

    return ast;
}

// -*- fname(args)
static Ast* _cln_parse_call(Parser *parser){
    Object *ident = _cln_match(parser, TOK_IDENT);  // fname
    Ast *fun = cln_new_ast(AST_CALL, ident);
    _cln_match(parser, TOK_LPAREN);                 // (
    Ast *args = _cln_parse_arglist(parser);          // args
    _cln_match(parser, TOK_RPAREN);                 // )
    cln_ast_add_node(fun, args);
    return fun;
}

// -*-
static Ast* _cln_parse_arglist(Parser *parser){
    // - arg1, arg2, ...
    Ast *args = cln_new_ast(AST_EMPTY, CLN_NONE);
    while(parser->currentToken.tkind != TOK_RPAREN){
        Ast *arg = _cln_parse_value(parser);
        cln_ast_add_node(args, arg);
        if(parser->currentToken.tkind != TOK_RPAREN){
            _cln_match(parser, TOK_COMMA);
        }
    }

    return args;
}

// static Ast* _cln_parse_expr(Parser *parser);
// static Ast* _cln_parse_artith_expr(Parser *parser);
// static Ast* _cln_parse_term(Parser *parser);
// static Ast* _cln_parse_value(Parser *parser);
// static Ast* _cln_parse_return(Parser *parser);
// static Ast* _cln_parse_array(Parser *parser);
// static Ast* _cln_parse_object(Parser *parser);
// static Ast* _cln_parse_import(Parser *parser);
// static Ast* _cln_parse_load(Parser *parser);