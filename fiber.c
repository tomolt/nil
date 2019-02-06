#include <assert.h>
#include <stdlib.h>

#include "object.h"
#include "closure.h"
#include "vector.h"
#include "pair.h"
#include "environment.h"

#include "fiber.h"



/*
 * Code pointer functions
 *
 * Note: These functions are designed for speed, not
 * safety and error checking!
 */


void code_pointer_init(struct code_pointer *ptr)
{
    assert(ptr != NULL);   
    ptr->func = EMPTY_LIST;
    ptr->code = NULL;
    ptr->offset = 0;
}


void code_pointer_enter_func(struct code_pointer *ptr, objptr_t func)
{
    assert((ptr != NULL) && (is_of_type(func, &TYPE_CLOSURE_PROTOTYPE)));
    decrease_refcount(ptr->func);
    ptr->func = func;
    increase_refcount(func);
    ptr->code = &(((struct closure_prototype*) dereference(func))->code);
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
 * Continuation frame
 */


void init_continuation_frame(struct continuation_frame *cf)
{
    cf->clink = EMPTY_LIST;
    cf->stack_ptr = EMPTY_LIST;
    cf->environment = EMPTY_LIST;
    code_pointer_init(&(cf->instr_pointer));
}


void terminate_continuation_frame(struct continuation_frame *cf)
{
    decrease_refcount(cf->clink);
    cf->clink = EMPTY_LIST;
    decrease_refcount(cf->stack_ptr);
    decrease_refcount(cf->environment);
    code_pointer_terminate(&(cf->instr_pointer));
}


unsigned int continuation_frame_slot_count(struct continuation_frame *cf)
{
    return 4;
}


objptr_t continuation_frame_slot_accessor(struct continuation_frame *cf,
                                          unsigned int slot)
{
    switch (slot) {
    case 0: return cf->clink;
    case 1: return cf->stack_ptr;
    case 2: return cf->environment;
    case 3: return cf->instr_pointer.func;
    default: return EMPTY_LIST;
    }
}


bool continuation_frame_eqv(struct continuation_frame *c1,
                            struct continuation_frame *c2,
                            enum eqv_strictness strictness)
{
    return c1 == c2;
}


DEFTYPE(TYPE_CONTINUATION_FRAME,
        struct continuation_frame,
        init_continuation_frame,
        terminate_continuation_frame,
        continuation_frame_slot_count,
        continuation_frame_slot_accessor,
        continuation_frame_eqv);



/*
 * Fiber code
 */


void fiber_enter_closure(struct fiber *fib, objptr_t closure)
{
    struct closure *instance;
    
    assert(fib != NULL && is_of_type(closure, &TYPE_CLOSURE));

    instance = (struct closure*) dereference(closure);

    fib->environment = instance->environment;
    increase_refcount(fib->environment);
    code_pointer_enter_func(&(fib->instr_pointer),
                            instance->prototype);
}


void fiber_init(struct fiber *fib, objptr_t thunk)
{
    assert(fib != NULL);

    fib->clink = EMPTY_LIST;
    fib->stack_ptr = EMPTY_LIST;
    fiber_enter_closure(fib, thunk);
    fib->environment = EMPTY_LIST;
    code_pointer_init(&(fib->instr_pointer));
    fiber_enter_closure(fib, thunk);
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
    increase_refcount(fib->stack_ptr);
}


static objptr_t fiber_pop(struct fiber *fib)
{
    objptr_t object;
    objptr_t next_elem;

    object = get_car(fib->stack_ptr);
    increase_refcount(object);
    next_elem = get_cdr(fib->stack_ptr);
    increase_refcount(next_elem);
    decrease_refcount(fib->stack_ptr);
    fib->stack_ptr = next_elem;
    return object;
}


static objptr_t fiber_get_continuation(struct fiber *fib)
{
    objptr_t continuation;
    struct continuation_frame *cf;

    continuation = object_allocate(&TYPE_CONTINUATION_FRAME);

    if (continuation != EMPTY_LIST) {
        cf = (struct continuation_frame*) dereference(continuation);
        cf->clink = fib->clink;             increase_refcount(cf->clink);
        cf->stack_ptr = fib->stack_ptr;     increase_refcount(cf->stack_ptr);
        cf->environment = fib->environment; increase_refcount(cf->environment);
        code_pointer_copy(&(cf->instr_pointer), &(fib->instr_pointer));
    }

    return continuation;
}


static objptr_t fiber_unwrap_params(struct fiber *fib,
                                    unsigned int given_var_count,
                                    objptr_t variable_name_vector,
                                    objptr_t rest_parameter_name)
{
    int i;
    unsigned int named_variable_count;
    unsigned int rest_variable_count;
    objptr_t object;
    objptr_t rest_parameter_list;
    objptr_t environment;

    /*
     * Set parameter counts
     */
    named_variable_count = vector_length(variable_name_vector);
    if (given_var_count < named_variable_count) {
        return EMPTY_LIST;
    }
    rest_variable_count = given_var_count - named_variable_count;
    
    /*
     * Push a new environment
     */
    environment = object_allocate(&TYPE_ENVIRONMENT);
    if (environment == EMPTY_LIST) {
        return EMPTY_LIST;
    }
    environment_set_parent(environment, fib->environment);

    /*
     * Bind rest parameters
     */
    if (rest_parameter_name != EMPTY_LIST) {
        rest_parameter_list = EMPTY_LIST;
        
        for (i = 0; i < rest_variable_count; i++)
        {
            object = fiber_pop(fib);
            rest_parameter_list = cons(object, rest_parameter_list);
            decrease_refcount(object);
        }

        environment_bind(environment, rest_parameter_name, rest_parameter_list);
    }

    /*
     * Bind named parameters
     */
    for (i = named_variable_count - 1; i >= 0; i--)
    {
        object = fiber_pop(fib);
        environment_bind(environment,
                         vector_get(variable_name_vector, i),
                         object);
        decrease_refcount(object);
    }
    
    return environment;
}



/*
 * Bytecode interpreter
 */

void fiber_tick(struct fiber *fib)
{
    /*
     * This is the magic bytecode execution function!
     * Please note that it's optimized for speed, not safety.
     */

    instr_t instruction;
    unsigned char opcode;
    unsigned int argument;
    objptr_t object, func;
    struct continuation_frame *frame;
    struct closure *closure;
    struct closure_prototype *closure_prototype;

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
	fiber_push(fib,
                   code_pointer_get_constant(&(fib->instr_pointer), argument));
	break;

    case INSTR_LOOKUP_CONST:
	object = code_pointer_get_constant(&(fib->instr_pointer), argument);
	fiber_push(fib, environment_get_binding(fib->environment, object));
	break;

    case INSTR_JMP:
	code_pointer_jump(&(fib->instr_pointer), argument);
	break;

    case INSTR_JMP_IF_NOT:
	object = fiber_pop(fib);
	if (object != NIL_FALSE) {
	    code_pointer_jump(&(fib->instr_pointer), argument);
	}
	decrease_refcount(object);
	break;

    case INSTR_CALL:
        object = fiber_get_continuation(fib);
        // We can do this since the continuation definitely contains a link
        // to the older clink
        decrease_refcount(fib->clink);
        fib->clink = object;
        increase_refcount(fib->clink);
    case INSTR_TAILCALL:
        func = fiber_pop(fib);
        if (is_of_type(func, &TYPE_CLOSURE)) {
            closure = (struct closure*) dereference(func);

            // Replace environment
            decrease_refcount(fib->environment);
            fib->environment = closure->environment;
            increase_refcount(fib->environment);

            if (!is_of_type(closure->prototype, &TYPE_CLOSURE_PROTOTYPE)) {
                // XXX: error: invalid closure!
            }

            closure_prototype =
                (struct closure_prototype*) dereference(closure->prototype);

            // Unpack parameters
            object = fiber_unwrap_params(fib,
                                         argument,
                                         closure_prototype->parameter_vector,
                                         closure_prototype->rest_parameter);

            // Set code pointer
            code_pointer_enter_func(&(fib->instr_pointer), closure->prototype);
        } else {
            // XXX: error: can't call this!
        }
        decrease_refcount(func);
	break;

    case INSTR_SET_CONST:
        object = fiber_pop(fib);
        environment_bind(fib->environment,
                         code_pointer_get_constant(&(fib->instr_pointer),
                                                   argument),
                         object);
        decrease_refcount(object);
	break;

    case INSTR_DEFINE_CONST:
        object = fiber_pop(fib);
        environment_bind(fib->environment,
                         code_pointer_get_constant(&(fib->instr_pointer),
                                                   argument),
                         object);
        decrease_refcount(object);
	break;

    case INSTR_POP:
        for (unsigned int i = 0; i < argument; i++)
        {
            decrease_refcount(fiber_pop(fib));
        }
	break;

    case INSTR_MAKE_CLOSURE:
        object = code_pointer_get_constant(&(fib->instr_pointer), argument);
        fiber_push(fib, make_closure_from_prototype(object, fib->environment));
	break;

    default:
	// TODO
	break;
    }
}
