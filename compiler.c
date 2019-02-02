#include <assert.h>
#include <stdlib.h>

#include "closure.h"
#include "symbol.h"
#include "pair.h"
#include "vector.h"

#include "compiler.h"



/*
 * Code for "struct code"
 */


unsigned int code_push_instruction(struct code *code,
                                   instr_t instruction)
{
    unsigned int new_alloc;

    assert(code != NULL);

    if (code->code_alloc >= code->code_size) {        
        if (code->code_alloc == 0) {
            new_alloc = 8;
        } else {
            new_alloc = code->code_alloc * 2;
        }

        code->codes = realloc(code->codes, new_alloc * sizeof(instr_t));
        // FIXME: Handle realloc() failures
        code->code_alloc = new_alloc;
    }

    code->codes[code->code_size] = instruction;
    
    return code->code_size++;
}


void code_set_instruction(struct code *code,
                          instr_t instruction,
                          unsigned int pos)
{
    /*
     * Note: Does not change the allocated memory,
     * the function ignores out-of-bounds arguments!
     */
    
    assert(code != NULL);

    if (pos < code->code_alloc) {
        code->codes[pos] = instruction;
    }
}


unsigned int code_add_constant(struct code *code, objptr_t constant)
{
    unsigned int pos;
    
    assert(code != NULL);

    if (code->constant_vector == EMPTY_LIST) {
        code->constant_vector = object_allocate(&TYPE_VECTOR);
        // FIXME: Handle allocation failures
    }

    // TODO: Check whether the object already is in the vector,
    // so that we can avoid storing multiple instances in the
    // same vector.
    
    pos = vector_length(code->constant_vector);
    vector_append(code->constant_vector, constant);

    return pos;
}


unsigned int code_get_write_location(struct code *code)
{
    assert(code != NULL);
    return code->code_size;
}


void init_code(struct code *code)
{
    code->constant_vector = EMPTY_LIST;
    code->code_size = 0;
    code->code_alloc = 0;
    code->codes = NULL;
}


void terminate_code(struct code *code)
{
    decrease_refcount(code->constant_vector);
    code->constant_vector = EMPTY_LIST;
    
    if (code->codes != NULL) {
	free(code->codes);
	code->codes = NULL;
    }
}



/*
 * Compiler
 */


static void compile_expression(objptr_t, struct code*, bool);

static unsigned int compile_parameter_list(objptr_t params,
                                           struct code *code)
{
    unsigned int param_count;

    for (param_count = 0; is_of_type(params, &TYPE_PAIR); param_count++)
    {
        compile_expression(get_car(params), code, false);
        params = get_cdr(params);
    }

    return param_count;
}


static void compile_begin(objptr_t expr_list,
                          struct code *code,
                          bool enable_tailcall)
{
    while (is_of_type(expr_list, &TYPE_PAIR))
    {
        if (is_of_type(get_cdr(expr_list), &TYPE_PAIR)) {
            /*
             * This code is located in the middle of a BEGIN
             * block, so we have to pop the return value.
             */
            compile_expression(get_car(expr_list), code, false);
            code_push_instruction(code, INSTRUCTION(INSTR_POP, 1));
        } else {
            /*
             * This is the last element in the block, so we can
             * use tailcall elimination here.
             */
            compile_expression(get_car(expr_list), code, enable_tailcall);
        }
        expr_list = get_cdr(expr_list);
    }
}


static objptr_t compile_lambda_prototype(objptr_t params, objptr_t body)
{
    objptr_t func;
    struct closure *instance;
    
    func = make_closure_prototype(params);

    if (func != EMPTY_LIST) {
        instance = (struct closure*) dereference(func);
        compile_begin(body, &(instance->code), true);
    }

    return func;
}


static void compile_expression(objptr_t expr,
                               struct code *code,
                               bool enable_tailcall)
/*
 * TODO: Add another parameter: no_stack_effect, for use
 * in BEGIN blocks. This parameter will also have to be
 * implemented in all other compiler subroutines.
 */
{
    objptr_t car;
    objptr_t cdr;
    objptr_t cadr;
    objptr_t caddr;
    unsigned int pos;
    unsigned int param_count;
    
    if (is_of_type(expr, &TYPE_SYMBOL)) {
        /*
         * Symbols will be looked up.
         */
        pos = code_add_constant(code, expr);
        code_push_instruction(code, INSTRUCTION(INSTR_LOOKUP_CONST, pos));
        
    } else if (is_of_type(expr, &TYPE_PAIR)) {
        /*
         * This is either a function call, or a builtin special form.
         */
        
        car = get_car(expr);
        cdr = get_cdr(expr);

        if (car == SYMBOL_QUOTE) {
            pos = code_add_constant(code, get_car(get_cdr(expr)));
            code_push_instruction(code, INSTRUCTION(INSTR_PUSH_CONST, pos));
            
        } else if (car == SYMBOL_SETBANG) {
            cadr = get_car(get_cdr(expr));
            caddr = get_car(get_cdr(get_cdr(expr)));
            
            if (cadr != EMPTY_LIST) {
                pos = code_add_constant(code, cadr);
                compile_expression(caddr, code, false);
                code_push_instruction(code, INSTRUCTION(INSTR_SET_CONST, pos));
            } // TODO: else: error!

        } else if (car == SYMBOL_DEFINE) {
            cadr = get_car(get_cdr(expr));

	    if (is_of_type(cadr, &TYPE_PAIR)) {
		/*
		 * This is the (define (func . args) . body) part
		 */
		objptr_t func;
		pos = code_add_constant(code, get_car(cadr));
		func = compile_lambda_prototype(get_cdr(cadr), get_cdr(get_cdr(expr)));
		code_push_instruction(code, INSTRUCTION(INSTR_MAKE_CLOSURE,
							code_add_constant(code, func)));
		code_push_instruction(code, INSTRUCTION(INSTR_DEFINE_CONST, pos));
	    } else if (cadr != EMPTY_LIST) {
		/*
		 * This is the (define foo bar) part
		 */
		caddr = get_car(get_cdr(get_cdr(expr)));
                pos = code_add_constant(code, cadr);
                compile_expression(caddr, code, false);
                code_push_instruction(code,
                                      INSTRUCTION(INSTR_DEFINE_CONST, pos));
            } // TODO: else: error!
            
        } else if (car == SYMBOL_IF) {
            unsigned int jmploc;
            unsigned int endloc;
            objptr_t condition;
            objptr_t ifclause;
            objptr_t elseclause;
            
            condition = get_car(get_cdr(expr));
            ifclause = get_car(get_cdr(get_cdr(expr)));
            
            if (!is_of_type(get_cdr(get_cdr(expr)), &TYPE_PAIR)) {
                /*
                 * No else clause
                 */
                elseclause = NIL_FALSE;
            } else {
                elseclause = get_car(get_cdr(get_cdr(expr)));
            }

            compile_expression(condition, code, false);
            jmploc = code_push_instruction(code,
                                           INSTRUCTION(INSTR_JMP_IF_NOT, ~0));
            compile_expression(ifclause, code, enable_tailcall);
            endloc = code_push_instruction(code,
                                           INSTRUCTION(INSTR_JMP, ~0));
            code_set_instruction(code,
                                 jmploc,
                                 INSTRUCTION(INSTR_JMP_IF_NOT, endloc + 1));
            compile_expression(elseclause, code, enable_tailcall);
            code_set_instruction(code,
                                 endloc,
                                 INSTRUCTION(INSTR_JMP,
                                             code_get_write_location(code)));

        } else if (car == SYMBOL_BEGIN) {
            compile_begin(get_cdr(expr), code, enable_tailcall);
            
        } else if (car == SYMBOL_LAMBDA) {
            objptr_t param_list;
            objptr_t body;
            objptr_t func;

            param_list = get_car(get_cdr(expr));
            body = get_car(get_cdr(get_cdr(expr)));

            func = compile_lambda_prototype(param_list, body);
            pos = code_add_constant(code, func);
            code_push_instruction(code, INSTRUCTION(INSTR_MAKE_CLOSURE, pos));
            
        } else {
            /*
             * No builtin special form has matched, so we compile
             * a basic function call.
             */
            compile_expression(car, code, false);
            param_count = compile_parameter_list(cdr, code);
            if (enable_tailcall) {
                code_push_instruction(code,
                                      INSTRUCTION(INSTR_TAILCALL,
                                                  param_count));
            } else {
                code_push_instruction(code,
                                      INSTRUCTION(INSTR_CALL,
                                                  param_count));
            }
        }
    } else {
        /*
         * All other objects evaluate to themselves.
         */
        pos = code_add_constant(code, expr);
        code_push_instruction(code, INSTRUCTION(INSTR_PUSH_CONST, pos));
    }
}


void compile(objptr_t expr, struct code *code)
{
    compile_expression(expr, code, true);
}
