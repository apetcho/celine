#ifndef CELINE_H
#define CELINE_H

#include<stddef.h>
#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>

#define CLN_MAX_NUMID       255
#define CLN_RETURN_ID       0
#define CLN_SELF_ID         1


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

// -*-----------------------------------------------------------------*-
// -*- Symtable -> (IDTable)                                         -*-
// -*-----------------------------------------------------------------*-
// -*- IDTable
typedef struct{
    char* symbols[CLN_MAX_NUMID];
    uint32_t nsymbol;               // count
} Symtable;


// -*-
Symtable* cln_new_symtable();
uint32_t cln_get_symbol_index(const Symtable* table, const char* symbol);

#endif