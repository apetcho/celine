#include<ctype.h>
#include<string.h>

#include "celine.h"

#define CLN_EOF             '\0'
#define CLN_ARRAYLEN(arr)   (sizeof(arr)/sizeof(arr[0]))

/*
-*- Prelude -*-
Iterator
Object::new()
Integer::new().{Arithmethic, Math, BitOps}
Float::new().{Arithmetic, Math}
String::new().{STRING_API}
Array::new().{ARRAY_API}
Tuple::new().{TUPLE_API}
List::new().{LIST_API}
Set::new().{SET_API}
Dict::new().{DICT_API}
HashMap::new().{HASHMAP_API}

-*- stdlib -*-
-> random
-> datetime
-> fs
-> ...
import name
load "name"
foreach(var: vars){
    ...
}
while(boolExpr){}
if(boolExpr){}else[if]{}
def funame(argv){}
let obj = new Fn(...)
obj.attr = ...
obj.prototype = ...

*/
// -*-
const char* clnKeywords[] = {
    "and", "or", "not",
    "while", "if", "else",
    "call", "print", "readInt",
    "def", "local", "return",
    "array", "object", "import",
    "new", "load"
};

enum TokenKind keywordsKind[] = {
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_WHILE, TOK_IF, TOK_ELSE,
    TOK_CALL, TOK_PRINT, TOK_READ_INT,
    TOK_INPUT, TOK_DEF, TOK_LOCAL,
    TOK_ARRAY, TOK_OBJECT, TOK_IMPORT,
    TOK_NEW, TOK_LOAD
};

char clnDelimiters[] = {
    ';', '=', '+', '-', '*', '/',
    '(', ')', '[', ']', ',', CLN_EOF
};

enum TokenKind clnDelimitersKind[] = {
    TOK_SEMI, TOK_ASSIGN, TOK_PLUS, TOK_MINUS,
    TOK_STAR, TOK_SLASH, TOK_LBRACE, TOK_RBRACE,
    TOK_LSBRACKET, TOK_RSBRACKET, TOK_COMMA, TOK_EOF,
};

// -*---------------------------------------------------------------*-
// -*- Ast                                                         -*-
// -*---------------------------------------------------------------*-

const char* clnTokenNames[] = {
#define CLN_DEF(kind, name)     name,
    CLN_TOKENS
#undef CLN_DEF
};

// -*-
// fail()
static void _cln_fail(Lexer *lexer, const char *message){
    fprintf(stderr, "CelineError: %s at [%d]\n", message, lexer->offset);
    cln_lexer_destroy(lexer);
    exit(EXIT_FAILURE);
}

// fail_with_invalid_symbol()
static void _cln_fail_with_invalid_symbol(Lexer *lexer, char expected, char got){
    fprintf(
        stderr, "CelineError: expected '%c', got '%c' at [%d]\n",
        expected, got, lexer->offset
    );
    cln_lexer_destroy(lexer);
    exit(EXIT_FAILURE);
}

// -*-
void cln_lexer_init(Lexer *lexer, const char *filename, Symtable *symtable){
    lexer->stream = NULL;
    lexer->pos = 0;
    lexer->offset = 0;
    lexer->symtable = symtable;
    memset(lexer->buffer, 0, sizeof(char)*CLN_BUFSIZE);
    memset(lexer->token, 0, sizeof(char)*CLN_MAX_TOKLEN);
    lexer->stream = fopen(filename, "r");
    if(!lexer->stream){
        _cln_fail(lexer, "Failed to open an input stream");
    }
    lexer->bufsize = fread(lexer->stream, sizeof(char), CLN_BUFSIZE, lexer->stream);
    lexer->lineno = 1;
    lexer->nextIsFieldName = false;
}

// nextchar()
static char _cln_nextchar(Lexer *lexer){
    return lexer->buffer[lexer->pos];
}

// nextchar_and_advance()
static char _cln_nextchar_and_advance(Lexer *lexer){
    char c = _cln_nextchar(lexer);
    if(lexer->pos == lexer->bufsize){
        if(feof(lexer->stream)){
            return CLN_EOF;
        }else{
            lexer->bufsize = fread(lexer->buffer, sizeof(char), CLN_BUFSIZE, lexer->stream);
            lexer->pos = 0;
        }
    }
    ++lexer->pos;
    ++lexer->offset;
    return c;
}

// advance_(head|pos)()
static void _cln_advance_pos(Lexer *lexer){
    ++lexer->pos;
    ++lexer->offset;
}

// skip_whitespace()
static void _cln_skip_whitespace(Lexer *lexer){
    bool comment = false;
    char c;
    while((c=_cln_nextchar(lexer)) && (isspace(c)||c==CLN_COMMENT || comment)){
        if(c==CLN_COMMENT){
            comment = true;
        }
        if(c=='\n'){
            ++lexer->lineno;
            comment = false;
        }
        _cln_advance_pos(lexer);
    }
}

// clear_(token|tokenBuffer)()
static void _cln_clear_token(Lexer *lexer){
    memset(lexer->token, '\0', sizeof(char)*CLN_MAX_TOKLEN);
}

// readSymbols()
static void _cln_read_symbol(Lexer *lexer, bool (*testfn)(char)){
    _cln_clear_token(lexer);
    int idx = 0;
    char c;
    do{
        c = _cln_nextchar(lexer);
        if(!testfn(c)){ break; }
        lexer->token[idx++] = c;
        _cln_advance_pos(lexer);
    }while(idx < CLN_MAX_TOKLEN);
    lexer->token[idx] = '\0';

}

// is_ident_symbol()
static bool _cln_is_ident_symbol(char c){
    return isalnum(c) || c == '_';
}

// ::is_number()
static bool _cln_is_number_symbol(char c){
    return isdigit(c) || c == '.' || c == 'e' || c == '+' || c == '-';
}

// read_ident()
// is_not_string_end()
// read_string_literal()
// get_keyword_token()
Token cln_lexer_nextoken(Lexer *lexer);
void cln_lexer_destroy(Lexer *lexer);
// bool cln_lexer_has_nextotken(Lexer *lexer);