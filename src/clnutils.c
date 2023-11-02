#include<string.h>

#include "celine.h"


// -*-
Symtable* cln_new_symtable(){
    Symtable *symtable = (Symtable*)cln_alloc(sizeof(Symtable));
    memset(symtable->symbols, 0, sizeof(char*)*CLN_MAX_NUMID);
    symtable->symbols[CLN_RETURN_ID] = "__return__";
    symtable->symbols[CLN_SELF_ID] = "self";
    symtable->len = 2;
    return symtable;
}

// -*-
uint32_t cln_get_symbol_index(Symtable* table, const char* symbol){
    for(uint32_t idx=0; idx < table->len; ++idx){
        if(strcmp(symbol, table->symbols[idx])==0){
            return idx;
        }
    }
    uint32_t index = table->len;
    table->symbols[index] = strdup(symbol);
    table->len++;
    return index;
}