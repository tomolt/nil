#include <assert.h>
#include <stdlib.h>

#include "object.h"
#include "closure.h"
#include "vector.h"
#include "pair.h"

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


void code_pointer_copy(struct code_pointer *dst, struct code_pointer *src)
{
    /*
     * This assigns SRC to DST. DST may already be initialized.
     */

    // Copy func
    decrease_refcount(dst->func);
    dst->func = src->func;
    increase_refcount(dst->func);

    // Copy code
    dst->code = src->code;

    // Copy offset
    dst->offset = src->offset;
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




/*
 * Fiber code
 */


void fiber_init(struct fiber *fib, objptr_t thunk)
{
    assert(fib != NULL);

    fib->clink = EMPTY_LIST;
    fib->stack_ptr = EMPTY_LIST;
    fib->environment = EMPTY_LIST;
    code_pointer_init_from_func(&(fib->instr_pointer), thunk);
}


void fiber_terminate(struct fiber *fib)
{
    assert(fib != NULL);

    decrease_refcount(fib->clink);        fib->clink = EMPTY_LIST;
    decrease_refcount(fib->stack_ptr);    fib->stack_ptr = EMPTY_LIST;
    decrease_refcount(fib->environment);  fib->environment = EMPTY_LIST;
    code_pointer_terminate(&(fib->instr_pointer));
}



static void fiber_push(struct fiber *fib, objptr_t obj)
{
    objptr_t new_stack;

    new_stack = cons(obj, fib->stack_ptr);
    decrease_refcount(fib->stack_ptr);
    fib->stack_ptr = new_stack;
}


void fiber_tick(struct fiber *fib)
{
    /*
     * This is the magic bytecode execution function!
     * Please note that it's optimized for speed, not safety.
     */

    instr_t instruction;
    unsigned char opcode;
    unsigned int argument;
    struct continuation_frame *frame;
    objptr_t object;

    /*
     * Return from functions
     */
    while (!code_pointer_is_valid(&(fib->instr_pointer)))
    {
	if (fib->clink == EMPTY_LIST) {
	    // TODO: Set variable in fiber that no code is
	    //       available
	    return;
	} else {
	    /*
	     * Pop frame
	     */
	    frame = (struct continuation_frame*) dereference(fib->clink);

	    // Restore environment
	    decrease_refcount(fib->environment);
	    fib->environment = frame->environment;

	    // Restore code pointer
	    code_pointer_copy(&(fib->instr_pointer), &(frame->instr_pointer));

	    // We can now pop clink and delete the old frame
	    object = fib->clink;
	    fib->clink = frame->clink;
	    increase_refcount(fib->clink);
	    decrease_refcount(object);
	}
    }


    
    /*
     * Let's start interpreting bytecodes!
     */
    
    instruction = code_pointer_get(&(fib->instr_pointer));
    opcode = INSTRUCTION_PART(instruction);
    argument = ARGUMENT_PART(instruction);

    switch (opcode) {
    case INSTR_HALT:
	// TODO: Set variable in fib that fiber has halted
	break;

    case INSTR_PUSH_CONST:
	fiber_push(fib, code_pointer_get_constant(&(fib->instr_pointer), argument));
	break;

	// TODO!
    }
}
