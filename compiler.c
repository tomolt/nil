#include <assert.h>
#include <stdlib.h>

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


unsigned int code_add_constant(struct code *code, objptr_t constant)
{
    unsigned int pos;
    
    assert(code != NULL);

    if (code->constant_vector == EMPTY_LIST) {
        code->constant_vector = object_allocate(&TYPE_VECTOR);
    }

    // FIXME: Handle allocation failures

    pos = vector_length(code->constant_vector);
    vector_append(code->constant_vector, constant);

    return pos;
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

    for (param_count = 0; is_of_type(params, &TYPE_PAIR); param_count++) {
        compile_expression(get_car(params), code, false);
        params = get_cdr(params);
    }

    return param_count;
}


static void compile_expression(objptr_t expr,
                               struct code *code,
                               bool enable_tailcall)
{
    objptr_t car;
    objptr_t cdr;
    unsigned int pos;
    unsigned int param_count;
    
    if (is_of_type(expr, &TYPE_SYMBOL)) {
        pos = code_add_constant(code, expr);
        code_push_instruction(code, INSTRUCTION(INSTR_LOOKUP_CONST, pos));
    } else if (is_of_type(expr, &TYPE_PAIR)) {
        car = get_car(expr);
        cdr = get_cdr(expr);

        if (car == SYMBOL_QUOTE) {
            pos = code_add_constant(code, get_car(get_cdr(expr)));
            code_push_instruction(code, INSTRUCTION(INSTR_PUSH_CONST, pos));
            // TODO: Add lambda, set!, let, define, ...
        } else {
            compile_expression(car, code, false);
            param_count = compile_parameter_list(cdr, code);
            if (enable_tailcall) {
                code_push_instruction(code, INSTRUCTION(INSTR_TAILCALL,
                                                        param_count));
            } else {
                code_push_instruction(code, INSTRUCTION(INSTR_CALL,
                                                        param_count));
            }
        }
    }
}


void compile(objptr_t expr, struct code *code)
{
    compile_expression(expr, code, true);
}
