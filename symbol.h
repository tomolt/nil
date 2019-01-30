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

extern objptr_t SYMBOL_DEFINE;
extern objptr_t SYMBOL_QUOTE;
extern objptr_t SYMBOL_LAMBDA;
extern objptr_t SYMBOL_LET;
extern objptr_t SYMBOL_SETBANG;
extern objptr_t SYMBOL_IF;
extern objptr_t SYMBOL_BEGIN;

objptr_t c_string_to_symbol(const char*);
objptr_t string_to_symbol(objptr_t);
objptr_t symbol_to_string(objptr_t);

void init_symbols();
void terminate_symbols();

#endif
