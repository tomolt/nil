
#include "number.h"


void init_number(struct number *number)
{
    number->type = NUMBER_INTEGER;
    number->value.integer = 0;
}


void terminate_number(struct number *number)
{
}


bool number_eqv(struct number *e1,
		struct number *e2,
		enum eqv_strictness strictness)
{
    if (e1 == e2) return true;
    
    if (e1->type != e2->type) {
	switch (e1->type) {
	case NUMBER_INTEGER:
	    if (e1->value.integer == e2->value.integer) {
		return true;
	    }
	    break;
	case NUMBER_REAL:
	    if (e1->value.real == e2->value.real) {
		return true;
	    }
	    break;
	case NUMBER_RATIONAL:
	    if (e1->value.rational.numerator == e2->value.rational.numerator
		&& e1->value.rational.denominator == e2->value.rational.denominator) {
		return true;
	    }
	    break;
	case NUMBER_COMPLEX:
	    if (e1->value.complex.real == e2->value.complex.real
		&& e1->value.complex.imaginary == e2->value.complex.imaginary) {
		return true;
	    }
	    break;
	}
    }

    /*
     * TODO:
     * if (strictness == EQV_STRICTNESS) return false;
     * if (strictness == EQ_STRICTNESS || strictness == EQUAL_STRICTNESS) {
     *    // compare integer/real, ...
     * }
     */
    
    return false;
}


DEFTYPE(TYPE_NUMBER,
	struct number,
	init_number,
	terminate_number,
	NULL,
	NULL,
	number_eqv);

/*
objptr_t make_integer(int);
objptr_t make_real(double);
objptr_t make_rational(int, unsigned int);
objptr_t make_complex(double, double);
*/
