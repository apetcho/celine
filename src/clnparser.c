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
static Ast* _cln_parse_assign(Parser *parser);
static Ast* _cln_parse_while(Parser *parser);
static Ast* _cln_parse_if(Parser *parser);
static Ast* _cln_parse_call(Parser *parser);
static Ast* _cln_parse_arglis(Parser *parser);
static Ast* _cln_parse_expr(Parser *parser);
static Ast* _cln_parse_artith_expr(Parser *parser);
static Ast* _cln_parse_term(Parser *parser);
static Ast* _cln_parse_value(Parser *parser);
static Ast* _cln_parse_print(Parser *parser);
static Ast* _cln_parse_def(Parser *parser);
static Ast* _cln_parse_return(Parser *parser);
static Ast* _cln_parse_array(Parser *parser);
static Ast* _cln_parse_object(Parser *parser);
static Ast* _cln_parse_import(Parser *parser);
static Ast* _cln_parse_load(Parser *parser);

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
    }
    _cln_fail_with_unexpected_token(parser, parser->currentToken.tkind, TOK_IDENT);
    return ast;
}

//static Ast* _cln_parse_block(Parser *parser);