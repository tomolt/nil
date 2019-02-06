#include <stdlib.h>

#include "compiler.h"
#include "pair.h"
#include "vector.h"

#include "closure.h"



void init_closure_prototype(struct closure_prototype *cp)
{
    cp->is_macro = false;
    cp->parameter_vector = EMPTY_LIST;
    cp->rest_parameter = EMPTY_LIST;
    init_code(&(cp->code));
}


void terminate_closure_prototype(struct closure_prototype *cp)
{
    decrease_refcount(cp->parameter_vector);
    cp->parameter_vector = EMPTY_LIST;
    decrease_refcount(cp->rest_parameter);
    cp->rest_parameter = EMPTY_LIST;
    terminate_code(&(cp->code));
}


unsigned int closure_prototype_slot_count(struct closure_prototype *cp)
{
    return 3;
}


objptr_t closure_prototype_slot_accessor(struct closure_prototype *cp,
                                         unsigned int slot)
{
    switch (slot) {
    case 0: return cp->parameter_vector;
    case 1: return cp->rest_parameter;
    case 2: return cp->code.constant_vector;
    default: return EMPTY_LIST;
    }
}


bool closure_prototype_eqv(struct closure_prototype *c1,
                           struct closure_prototype *c2,
                           enum eqv_strictness strictness)
{
    return c1 == c2;
}


DEFTYPE(TYPE_CLOSURE_PROTOTYPE,
	struct closure_prototype,
	init_closure_prototype,
	terminate_closure_prototype,
	closure_prototype_slot_count,
	closure_prototype_slot_accessor,
	closure_prototype_eqv);



void init_closure(struct closure *closure)
{
    closure->prototype = EMPTY_LIST;
    closure->environment = EMPTY_LIST;
}


void terminate_closure(struct closure *closure)
{
    decrease_refcount(closure->prototype);
    closure->prototype = EMPTY_LIST;
    decrease_refcount(closure->environment);
    closure->environment = EMPTY_LIST;
}


unsigned int closure_slot_count(struct closure *closure)
{
    return 2;
}


objptr_t closure_slot_accessor(struct closure *closure,
                               unsigned int slot)
{
    switch (slot) {
    case 0: return closure->prototype;
    case 1: return closure->environment;
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
    struct closure_prototype *instance;

    ptr = object_allocate(&TYPE_CLOSURE_PROTOTYPE);

    if (ptr != EMPTY_LIST) {
        instance = (struct closure_prototype*) dereference(ptr);
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


objptr_t make_closure_from_prototype(objptr_t proto, objptr_t env)
{
    objptr_t ptr;
    struct closure *instance;
    
    ptr = object_allocate(&TYPE_CLOSURE);

    if (ptr != EMPTY_LIST) {
        instance = (struct closure*) dereference(ptr);
        instance->prototype = proto;
        increase_refcount(instance->prototype);
        instance->environment = env;
        increase_refcount(instance->environment);
    }
    
    return ptr;
}
