#include <stdlib.h>

#include "compiler.h"
#include "pair.h"
#include "vector.h"

#include "closure.h"



void init_closure(struct closure *closure)
{
    closure->is_macro = false;
    closure->parameter_vector = EMPTY_LIST;
    closure->rest_parameter = EMPTY_LIST;
    init_code(&(closure->code));
    closure->environment = EMPTY_LIST;
}


void terminate_closure(struct closure *closure)
{
    decrease_refcount(closure->parameter_vector);
    closure->parameter_vector = EMPTY_LIST;
    decrease_refcount(closure->rest_parameter);
    closure->rest_parameter = EMPTY_LIST;
    decrease_refcount(closure->environment);
    closure->environment = EMPTY_LIST;

    terminate_code(&(closure->code));
}


unsigned int closure_slot_count(struct closure *closure)
{
    return 4;
}


objptr_t closure_slot_accessor(struct closure *closure, unsigned int slot)
{
    switch (slot) {
    case 0: return closure->parameter_vector;
    case 1: return closure->rest_parameter;
    case 2: return closure->code.constant_vector;
    case 3: return closure->environment;
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


objptr_t make_closure_prototype(objptr_t params)
{
    objptr_t ptr;
    struct closure *instance;

    ptr = object_allocate(&TYPE_CLOSURE);

    if (ptr != EMPTY_LIST) {
        instance = (struct closure*) dereference(ptr);
        instance->parameter_vector = make_vector(EMPTY_LIST, 0);
        increase_refcount(instance->parameter_vector);

	while (is_of_type(params, &TYPE_PAIR))
	{
	    vector_append(instance->parameter_vector, get_car(params));
	    params = get_cdr(params);
	}

	instance->rest_parameter = params;
	increase_refcount(instance->rest_parameter);
    }

    return ptr;
}
