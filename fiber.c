#include <assert.h>
#include <stdlib.h>

#include "object.h"
#include "closure.h"
#include "vector.h"

#include "fiber.h"



/*
 * Code pointer functions
 *
 * Note: These functions are designed for speed, not
 * safety and error checking!
 */


void code_pointer_init_from_func(struct code_pointer *ptr, objptr_t func)
{
    assert((ptr != NULL) && (is_of_type(func, &TYPE_CLOSURE)));
    ptr->func = func;
    increase_refcount(func);
    ptr->code = &(((struct closure*) dereference(func))->code);
    ptr->offset = 0;
}


void code_pointer_terminate(struct code_pointer *ptr)
{
    assert(ptr != NULL);
    decrease_refcount(ptr->func);
}


instr_t code_pointer_get(struct code_pointer *ptr)
{
    if (code_pointer_is_valid(ptr)) {
	return ptr->code->codes[ptr->offset++];
    } else {
	return INSTRUCTION(INSTR_HALT, 0);
    }
}


objptr_t code_pointer_get_constant(struct code_pointer *ptr, unsigned int n)
{
    return vector_get(ptr->code->constant_vector, n);
}


void code_pointer_jump(struct code_pointer *ptr, unsigned int pos)
{
    ptr->offset = pos;
}


bool code_pointer_is_valid(struct code_pointer *ptr)
{
    return ptr->offset < ptr->code->code_size;
}
