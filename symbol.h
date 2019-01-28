#pragma once

#ifndef SYMBOL_H_
#define SYMBOL_H_


#include "object.h"


struct symbol {
    struct object head;

    objptr_t self;    
    struct symbol *symbol_table_prev;
    struct symbol *symbol_table_next;

    bool is_gensym;
    objptr_t name_string;
};


extern struct object_type TYPE_SYMBOL;


objptr_t c_string_to_symbol(const char*);
objptr_t string_to_symbol(objptr_t);
objptr_t symbol_to_string(objptr_t);
// TODO

void init_symbols();
void terminate_symbols();

#endif
