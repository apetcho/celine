#ifndef CELINE_H
#define CELINE_H

#include<stddef.h>
#include<stdint.h>

#define CLN_MAX_NUMID       255
#define CLN_RETURN_ID       0
#define CLN_SELF_ID         1

// -*- IDTable
typedef struct{
    char* symbols[CLN_MAX_NUMID];
    uint32_t nsymbol;               // count
} Symtable;


// -*-
Symtable* cln_new_symtable();
uint32_t cln_get_symbol_index(const Symtable* table, const char* symbol);

#endif