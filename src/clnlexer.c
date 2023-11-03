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

enum TokenKind clnKeywordsKind[] = {
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_WHILE, TOK_IF, TOK_ELSE,
    TOK_CALL, TOK_PRINT, TOK_READ_INT,
    TOK_INPUT, TOK_DEF, TOK_LOCAL,
    TOK_ARRAY, TOK_OBJECT, TOK_IMPORT,
    TOK_NEW, TOK_LOAD
};

char clnDelimiters[] = {
    ';', '=', '+', '-', '*', '/',
    '(', ')', '[', ']', '{', '}',
    ',', CLN_EOF
};

enum TokenKind clnDelimitersKind[] = {
    TOK_SEMI, TOK_ASSIGN, TOK_PLUS, TOK_MINUS,
    TOK_STAR, TOK_SLASH, TOK_LBRACE, TOK_RBRACE,
    TOK_LSBRACKET, TOK_RSBRACKET, TOK_LBRACE,
    TOK_RBRACE, TOK_COMMA, TOK_EOF,
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

// -*-
static bool _cln_is_not_string_end(char c){
    return c != '\"';
}

// read_ident()
static void _cln_read_symbol_tillws(Lexer *lexer){
    _cln_read_symbol(lexer, _cln_is_ident_symbol);
}

// read_string_literal()
static void _cln_read_string_literal(Lexer *lexer){
    _cln_read_symbol(lexer, _cln_is_not_string_end);
}

// read_number_literal()
static void _cln_read_number_literal(Lexer *lexer){
    _cln_read_symbol(lexer, _cln_is_number_symbol);
}

// get_keyword_token()
enum TokenKind _cln_get_keyword_token(char *cstr){
    size_t len = CLN_ARRAYLEN(clnKeywords);
    for(size_t i=0; i < len; ++i){
        if(strcmp(clnKeywords[i], cstr)==0){
            return clnKeywordsKind[i];
        }
    }
    return TOK_UNKNOWN;
}

// -*-
Token cln_lexer_nexttoken(Lexer *lexer){
    _cln_skip_whitespace(lexer);
    Token token;
    token.lineno = lexer->lineno;
    char c = _cln_nextchar(lexer);
    uint32_t pos = lexer->pos;
    uint32_t offset = lexer->offset;

    if(lexer->nextIsFieldName){
        _cln_read_symbol_tillws(lexer);
        char *str = (char*)cln_alloc(sizeof(char)*(strlen(lexer->token)+1));
        strcpy(str, lexer->token);
        token.tkind = TOK_FIELD;
        token.obj = cln_new_string(str);
        lexer->nextIsFieldName = false;
    }else if(c=='_' || isalpha(c)){ // ident or keyword: [A-Za-b_]+[A-Za-b0-9_]*
        _cln_read_symbol_tillws(lexer);
        enum TokenKind tkind = _cln_get_keyword_token(lexer->token);
        if(tkind == TOK_UNKNOWN){   // ident
            uint32_t idx = cln_get_symbol_index(lexer->symtable, lexer->token);
            token.tkind = tkind;
            token.obj = cln_new_integer(idx);
        }else{                      // keyword
            token.tkind = tkind;
        }
    }else if(((c=='-'|| c=='+') && (isdigit(_cln_nextchar(lexer))))){ // number literal
        lexer->pos = pos;
        lexer->offset = offset;
        _cln_read_number_literal(lexer);
        char *end = strchr(lexer->token, '.');
        if(end == NULL){
            token.obj = cln_new_integer(atol(lexer->token));
            token.tkind  = TOK_INTEGER;
        }else{
            char *ptr = NULL;
            double num = strtod(lexer->token, &ptr);
            token.obj = cln_new_float(num);
            token.tkind = TOK_FLOAT;
        }
    }else if(c=='\"'){  // string literal
        _cln_advance_pos(lexer);
        _cln_read_string_literal(lexer);
        char *str = (char*)cln_alloc(sizeof(char)*(strlen(lexer->token)+1));
        strcpy(str, lexer->token);
        token.tkind = TOK_STRING;
        token.obj = cln_new_string(str);
        _cln_advance_pos(lexer);
    }else if(c=='='){ // = | ==
        _cln_advance_pos(lexer);
        c = _cln_nextchar_and_advance(lexer);
        if(c == '='){
            token.tkind = TOK_EQ;
        }else{
            token.tkind = TOK_ASSIGN;
        }
    }else if(c=='<'){
        _cln_advance_pos(lexer);
        c = _cln_nextchar_and_advance(lexer);
        if(c=='='){
            token.tkind = TOK_LE;
        }else{
            token.tkind = TOK_LT;
        }
    }else if(c=='>'){
        _cln_advance_pos(lexer);
        c = _cln_nextchar_and_advance(lexer);
        if(c=='='){
            token.tkind = TOK_GE;
        }else{
            token.tkind = TOK_GT;
        }
    }else if(c=='.'){ // field
        _cln_advance_pos(lexer);
        lexer->nextIsFieldName = true;
        token.tkind = TOK_DOT;
    }else{  // DELIMITER | OP
        bool found = false;
        for(int i=0; i < CLN_ARRAYLEN(clnDelimiters); ++i){
            if(c==clnDelimiters[i]){
                found = true;
                _cln_advance_pos(lexer);
                token.tkind = clnDelimitersKind[i];
            }
        }
        if(!found){
            _cln_fail_with_invalid_symbol(lexer, '?', c);
        }
    }

    return token;
}

// -*-
void cln_lexer_destroy(Lexer *lexer){
    fclose(lexer->stream);
}
// bool cln_lexer_has_nextotken(Lexer *lexer);