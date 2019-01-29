#include <stdlib.h>

#include "closure.h"


void init_closure(struct closure *closure)
{
    closure->parameter_list = EMPTY_LIST;
    closure->code.constant_vector = EMPTY_LIST;
    closure->code.code_size = 0;
    closure->code.code_alloc = 0;
    closure->code.codes = NULL;
    closure->environment = EMPTY_LIST;
}


void terminate_closure(struct closure *closure)
{
    decrease_refcount(closure->parameter_list);
    closure->parameter_list = EMPTY_LIST;
    decrease_refcount(closure->code.constant_vector);
    closure->code.constant_vector = EMPTY_LIST;
    decrease_refcount(closure->environment);
    closure->environment = EMPTY_LIST;

    if (closure->code.codes != NULL) {
	free(closure->code.codes);
	closure->code.codes = NULL;
    }
}


unsigned int closure_slot_count(struct closure *closure)
{
    return 3;
}


objptr_t closure_slot_accessor(struct closure *closure, unsigned int slot)
{
    switch (slot) {
    case 0: return closure->parameter_list;
    case 1: return closure->code.constant_vector;
    case 2: return closure->environment;
    default: return EMPTY_LIST;
    }
}


bool closure_eqv(struct closure *c1,
		 struct closure *c2,
		 enum eqv_strictness strictness)
{
    return c1 == c2;
}


DEFTYPE(TYPE_CLOSURE,
	struct closure,
	init_closure,
	terminate_closure,
	closure_slot_count,
	closure_slot_accessor,
	closure_eqv);


// TODO
