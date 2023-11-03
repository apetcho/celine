#ifndef CELINE_H
#define CELINE_H

#include<stdbool.h>
#include<stddef.h>
#include<stdarg.h>
#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>

#define CLN_MAX_NUMID           255
#define CLN_MAX_IDENT           255
#define CLN_MAX_TOKLEN          255
#define CLN_BUFSIZE             4096
#define CLN_RETURN_ID           0
#define CLN_SELF_ID             1
#define CLN_BUILTIN_MAXARGS     10
#define CLN_PROTOTYPE           "prototype"
#define CLN_COMMENT             '#'

// -*-
inline void* cln_alloc(size_t size){
    void* ptr = calloc(1, size);
    if(ptr==NULL){
        fprintf(stderr, "CelineError: memory allocation failure\n");
        exit(EXIT_FAILURE);        
    }
    return ptr;
}

// -*-
inline void cln_dealloc(void *ptr){
    if(ptr){
        free(ptr);
    }
    ptr = NULL;
}

// -*-
inline void cln_panic(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

// -*-------------------------------------------*-
// -*- Forward Declarations and other typedefs -*-
// -*-------------------------------------------*-
typedef struct ast Ast;
typedef struct env Env;
typedef struct object Object;
typedef struct field Field;     // KeyValue
typedef struct path Path;
typedef void (*CFun)(Env*);

// -*-----------------------------------------------------------------*-
// -*- Symtable -> (IDTable)                                         -*-
// -*-----------------------------------------------------------------*-
// -*- IDTable
typedef struct{
    char* symbols[CLN_MAX_NUMID];
    uint32_t len;               // count
} Symtable;


// -*-
Symtable* cln_new_symtable();
uint32_t cln_get_symbol_index(Symtable* table, const char* symbol);

// -*-----------------------------------------------------------------*-
// -*- Type -> (IDTable)                                             -*-
// -*-----------------------------------------------------------------*-
enum Type{
    TY_INTEGER = 0,
    TY_FLOAT,           // <extension>
    TY_STRING,
    TY_ARRAY,
    TY_FUN,             // <FUN>
    TY_OBJECT,
    TY_CFUN,            // <Foreign Function>
};

// -
struct object{
    enum Type type;
    union{
        long integer;       // integer
        double real;        // float
        char* cstr;         // string
        struct{
            Object **data;  // data
            size_t len;     // size
        } array ;
        // - function -
        struct {
            int narg;
            int *args;
            Ast *code;
        } fun;
    }val;               // v
    Field **fields;
    size_t ftcap;       // field_table_length
    size_t nfield;
};

struct field{
    char *name;     // key
    Object* obj;
};

// -
void cln_checktype(Object *obj, enum Type type);
char* cln_toString(const Object *self);
Object* cln_new_integer(long num);
Object* cln_new_float(double num);
Object* cln_new_string(char *cstr);
Object* cln_new_fun(int *args, int narg, Ast *code);
Object* cln_new_array(size_t len);
Object* cln_new();
uint32_t cln_hash(const char* cstr, size_t tableLen);
void cln_set_field(Object *self, const char* name, Object *obj);
Object* cln_get_field(Object *self, const char* name);
Object* cln_get_field_generic(Object *self, const char* name, bool checkproto);

// -*---------------------------------------------------------------*-
// -*- Ast                                                         -*-
// -*---------------------------------------------------------------*-

#define CLN_AST_NODES           \
    CLN_DEF(ASSIGN, "=")        \
    CLN_DEF(IDENT, "ident")     \
    CLN_DEF(CONST, "const")     \
    CLN_DEF(ADD, "+")           \
    CLN_DEF(SUB, "-")           \
    CLN_DEF(MUL, "*")           \
    CLN_DEF(DIV, "/")           \
    CLN_DEF(WHILE, "while")     \
    CLN_DEF(IF, "if")           \
    CLN_DEF(CALL, "call")       \
    CLN_DEF(AND, "and")         \
    CLN_DEF(OR, "or")           \
    CLN_DEF(NOT, "not")         \
    CLN_DEF(LT, "<")            \
    CLN_DEF(EQ, "==")           \
    CLN_DEF(GT, ">")            \
    CLN_DEF(LE, "<=")           \
    CLN_DEF(GE, ">=")           \
    CLN_DEF(PRINT, "print")     \
    CLN_DEF(INPUT, "input")     \
    CLN_DEF(DEF, "def")         \
    CLN_DEF(LOCAL, "local")     \
    CLN_DEF(RETURN, "return")   \
    CLN_DEF(ARRAY, "array")     \
    CLN_DEF(INDEX, "[]")        \
    CLN_DEF(FIELD, "field")     \
    CLN_DEF(MCALL, "mcall")     \
    CLN_DEF(OBJECT, "object")   \
    CLN_DEF(IMPORT, "import")   \
    CLN_DEF(NEW, "new")         \
    CLN_DEF(LOAD, "load")       \
    CLN_DEF(NONE, "_none_")

// -*-
enum AstKind {
#define CLN_DEF(kind, name)     AST_##kind,
    CLN_AST_NODES
#undef CLN_DEF
};

#define CLN_NONE    NULL

// -*-
struct ast{
    enum AstKind akind;
    Object *obj;
    // -
    Ast *node;
    Ast *next;
    Ast *parent;
};

extern char* clnAstKindNames[];

Ast* cln_new_ast(enum AstKind, Object *obj);
void cln_ast_add_node(Ast *parent, Ast *node);
void cln_dump(Ast *ast);

// -*---------------------------------------------------------------*-
// -*- Env                                                         -*-
// -*---------------------------------------------------------------*-

struct env{
    Object *idents[CLN_MAX_IDENT];
    Env *parent;
};

Env* cln_new_env();
bool cln_env_contains(Env *env, int id);
Object* cln_env_get(Env *env, int id);
void cln_env_update(Env *env, int id, Object *obj); // set
void cln_env_put(Env *env, int id, Object *obj);


// -*---------------------------------------------------------------*-
// -*- Module                                                      -*-
// -*---------------------------------------------------------------*-
//! @todo: use hashtable & cache
struct path {
    const char* name;
    Path *next;
};

typedef struct{
    Path *paths;
} Modules;

// -*-
typedef void (*InitModuleFn)(Symtable*, Env*);

void cln_module_addpath(const char* name);
Ast* cln_module_import(const char* name, Symtable* symtable);
void cln_module_load(const char* name, Symtable *symbtable, Env *env);

// -*---------------------------------------------------------------*-
// -*- Ast                                                         -*-
// -*---------------------------------------------------------------*-
#define CLN_TOKENS                  \
    CLN_DEF(INVALID, "invalid")     \
    CLN_DEF(LBRACE, "{")            \
    CLN_DEF(RBRACE, "}")            \
    CLN_DEF(IDENT, "ident")         \
    CLN_DEF(ASSIGN, "=")            \
    CLN_DEF(NUMBER, "const")        \
    CLN_DEF(SEMI, ";")              \
    CLN_DEF(AND, "and")             \
    CLN_DEF(OR, "or")               \
    CLN_DEF(NOT, "not")             \
    CLN_DEF(LT, "<")                \
    CLN_DEF(EQ, "==")               \
    CLN_DEF(GT, ">")                \
    CLN_DEF(LE, "<=")               \
    CLN_DEF(GE, ">=")               \
    CLN_DEF(PLUS, "+")              \
    CLN_DEF(MINUS, "-")             \
    CLN_DEF(STAR, "*")              \
    CLN_DEF(SLASH, "/")             \
    CLN_DEF(LPAREN, "(")            \
    CLN_DEF(RPAREN, ")")            \
    CLN_DEF(WHILE, "while")         \
    CLN_DEF(IF, "if")               \
    CLN_DEF(ELSE, "else")           \
    CLN_DEF(EOF, "eof")             \
    CLN_DEF(CALL, "call")           \
    CLN_DEF(PRINT, "print")         \
    CLN_DEF(INPUT, "input")         \
    CLN_DEF(STRING, "string")       \
    CLN_DEF(DEF, "def")             \
    CLN_DEF(COMMA, ",")             \
    CLN_DEF(LOCAL, "local")         \
    CLN_DEF(RETURN, "return")       \
    CLN_DEF(ARRAY, "array")         \
    CLN_DEF(LSBRACKET, "[")         \
    CLN_DEF(RSBRACKET, "]")         \
    CLN_DEF(DOT, ".")               \
    CLN_DEF(FIELD, "field")         \
    CLN_DEF(OBJECT, "object")       \
    CLN_DEF(IMPORT, "import")       \
    CLN_DEF(NEW, "new")             \
    CLN_DEF(LOAD, "load")


enum TokenKind{
#define CLN_DEF(kind, name)     TOK_##kind,
    CLN_TOKENS
#undef CLN_DEF
};

extern char* clnTokenNames[];

typedef struct{
    enum TokenKind tkind;
    uint32_t lineno;
    Object *obj;
} Token;

typedef struct {
    FILE *stream;
    char buffer[CLN_BUFSIZE];       // stream_buffer
    char token[CLN_MAX_TOKLEN];     // token_buffer
    Symtable *symtable;             // id_table
    uint32_t pos;                   // head
    uint32_t offset;                // abolute_head_position;
    size_t bufsize;                 // buffer_size
    uint32_t lineno;                // line_num;
    bool nextIsFieldName;           // field_name_following
} Lexer;

void cln_lexer_init(Lexer *lexer, const char *filename, Symtable *symtable);
void cln_lexer_destroy(Lexer *lexer);
Token cln_lexer_nextoken(Lexer *lexer);
bool cln_lexer_has_nextotken(Lexer *lexer);

// -*---------------------------------------------------------------*-
// -*- Parser                                                      -*-
// -*---------------------------------------------------------------*-
Ast* cln_parse(const char* filename, Symtable *symtable);


#endif