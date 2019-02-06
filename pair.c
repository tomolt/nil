
#include "pair.h"


void init_pair(struct pair *pair)
{
    pair->car = EMPTY_LIST;
    pair->cdr = EMPTY_LIST;
}


void terminate_pair(struct pair *pair)
{
    decrease_refcount(pair->car);  pair->car = EMPTY_LIST;
    decrease_refcount(pair->cdr);  pair->cdr = EMPTY_LIST;
}


unsigned int pair_slot_count(struct pair *pair)
{
    return 2;
}


objptr_t pair_slot_accessor(struct pair *pair, unsigned int slot)
{
    if (slot == 0) {
	return pair->car;
    } else if (slot == 1) {
	return pair->cdr;
    } else {
	return EMPTY_LIST;
    }
}


bool pair_eqv(struct pair *e1,
	      struct pair *e2,
	      enum eqv_strictness strictness)
{
    switch (strictness) {
    case EQV_STRICT:
    case EQ_STRICT:
	return e1 == e2;
    case EQUAL_STRICT:
	return eqv(e1->car, e2->car, strictness)
	    && eqv(e1->cdr, e2->cdr, strictness);
    default:
	return false;
    }
}


DEFTYPE(TYPE_PAIR,
	struct pair,
	init_pair,
	terminate_pair,
	pair_slot_count,
	pair_slot_accessor,
	pair_eqv);



objptr_t get_car(objptr_t ptr)
{
    struct pair *pair;
    
    if (is_of_type(ptr, &TYPE_PAIR)) {
	pair = (struct pair*) dereference(ptr);
	return pair->car;
    } else {
	return EMPTY_LIST;  // TODO: Error?
    }
}


void set_car(objptr_t ptr, objptr_t car)
{
    struct pair *pair;
    
    if (is_of_type(ptr, &TYPE_PAIR)) {
	pair = (struct pair*) dereference(ptr);
	decrease_refcount(pair->car);
	pair->car = car;
	increase_refcount(car);
    }  // else: Error?
}


objptr_t get_cdr(objptr_t ptr)
{
    struct pair *pair;
    
    if (is_of_type(ptr, &TYPE_PAIR)) {
	pair = (struct pair*) dereference(ptr);
	return pair->cdr;
    } else {
	return EMPTY_LIST;  // TODO: Error?
    }
}


void set_cdr(objptr_t ptr, objptr_t cdr)
{
    struct pair *pair;
    
    if (is_of_type(ptr, &TYPE_PAIR)) {
	pair = (struct pair*) dereference(ptr);
	decrease_refcount(pair->cdr);
	pair->cdr = cdr;
	increase_refcount(cdr);
    }  // else: Error?
}



objptr_t cons(objptr_t car, objptr_t cdr)
{
    objptr_t pair;

    pair = object_allocate(&TYPE_PAIR);

    if (pair != EMPTY_LIST) {
	set_car(pair, car);
	set_cdr(pair, cdr);
    }  // else: Error?

    return pair;
}
