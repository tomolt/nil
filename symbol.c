#include <stdlib.h>


#include "vector.h"

#include "symbol.h"




static struct symbol *SYMBOL_TABLE;


objptr_t SYMBOL_DEFINE;
objptr_t SYMBOL_QUOTE;
objptr_t SYMBOL_LAMBDA;
objptr_t SYMBOL_LET;
objptr_t SYMBOL_SETBANG;
objptr_t SYMBOL_IF;
objptr_t SYMBOL_BEGIN;
objptr_t SYMBOL_COMPILE;


void init_symbol(struct symbol *symbol)
{    
    /*
     * Give the symbol an empty name.
     */
    symbol->is_gensym = false;
    symbol->name_string = EMPTY_LIST;

    /*
     * Insert the symbol into the symbol table.
     */
    symbol->self = EMPTY_LIST;
    if (SYMBOL_TABLE == NULL) {
	symbol->symbol_table_prev = symbol;
	symbol->symbol_table_next = symbol;
	SYMBOL_TABLE = symbol;
    } else {
	symbol->symbol_table_prev = SYMBOL_TABLE->symbol_table_prev;
	symbol->symbol_table_next = SYMBOL_TABLE;
	SYMBOL_TABLE->symbol_table_prev->symbol_table_next = symbol;
	SYMBOL_TABLE->symbol_table_prev = symbol;
	SYMBOL_TABLE = symbol;
    }
}


void terminate_symbol(struct symbol *symbol)
{
    /*
     * Remove symbol from symbol table
     */
    
    if (SYMBOL_TABLE == symbol) {
	if (symbol->symbol_table_prev == symbol) {
	    // We deleted the last symbol in the table
	    SYMBOL_TABLE = NULL;
	} else {
	    // The symbol table now points to the previous
	    // object
	    SYMBOL_TABLE = symbol->symbol_table_prev;
	}
    }

    if (symbol->symbol_table_prev != NULL) {
	symbol->symbol_table_prev->symbol_table_next = symbol->symbol_table_next;
    }
    
    if (symbol->symbol_table_next != NULL) {
	symbol->symbol_table_next->symbol_table_prev = symbol->symbol_table_prev;
    }
    

    /*
     * Delete the references
     */
    decrease_refcount(symbol->name_string);
}


unsigned int symbol_slot_count(struct symbol *symbol)
{
    return 1;
}


objptr_t symbol_slot_accessor(struct symbol *symbol, unsigned int slot)
{
    if (slot == 0) {
	return symbol->name_string;
    } else {
	return EMPTY_LIST;
    }
}


bool symbol_eqv(struct symbol *s1,
		struct symbol *s2,
		enum eqv_strictness strictness)
{
    return s1 == s2;
}


DEFTYPE(TYPE_SYMBOL,
	struct symbol,
	init_symbol,
	terminate_symbol,
	symbol_slot_count,
	symbol_slot_accessor,
	symbol_eqv);



objptr_t c_string_to_symbol(const char *str)
{
    return string_to_symbol(make_string_from_c_string(str));
}


objptr_t string_to_symbol(objptr_t name)
{
    objptr_t ptr;
    struct symbol *symbol;

    if (SYMBOL_TABLE != NULL) {
	symbol = SYMBOL_TABLE;
	do {
	    if (eqv(name, symbol->name_string, EQUAL_STRICT)) {
		return symbol->self;
	    }
	    symbol = symbol->symbol_table_next;
	} while(symbol != SYMBOL_TABLE);
    }

    ptr = object_allocate(&TYPE_SYMBOL);
    if (ptr == EMPTY_LIST) return ptr;
    symbol = (struct symbol*) dereference(ptr);
    
    symbol->self = ptr;
    symbol->name_string = vector_copy(name);
    increase_refcount(symbol->name_string);
    
    return ptr;
}


objptr_t symbol_to_string(objptr_t ptr)
{
    struct symbol *symbol;

    if (is_of_type(ptr, &TYPE_SYMBOL)) {
	symbol = (struct symbol*) dereference(ptr);
	return symbol->name_string;
    } else {
	return EMPTY_LIST;
    }
}




void init_global_symbol(objptr_t *slot, const char *name)
{
    *slot = c_string_to_symbol(name);
    declare_root_object(*slot);
    make_refcount_immune(*slot);
}


void init_symbols()
{
    SYMBOL_TABLE = NULL;

    // Init symbols
    init_global_symbol(&SYMBOL_DEFINE, "define");
    init_global_symbol(&SYMBOL_QUOTE, "quote");
    init_global_symbol(&SYMBOL_LAMBDA, "lambda");
    init_global_symbol(&SYMBOL_LET, "let");
    init_global_symbol(&SYMBOL_SETBANG, "set!");
    init_global_symbol(&SYMBOL_IF, "if");
    init_global_symbol(&SYMBOL_BEGIN, "begin");
    init_global_symbol(&SYMBOL_COMPILE, "compile");
}


void terminate_symbols()
{
    // The memory manager will free the instances automatically,
    // therefore we can simply clear the pool handle
    SYMBOL_TABLE = NULL;
    free_type_instances(&TYPE_SYMBOL);
}
