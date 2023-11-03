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
// match()
// 

// -*-
static Ast* _cln_parse_program(Parser *parser);
static Ast* _cln_parse_block(Parser *parser);
static Ast* _cln_parse_condition(Parser *parser);
static Ast* _cln_parse_logical_expr(Parser *parser);
static Ast* _cln_parse_oplist(Parser *parser);
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