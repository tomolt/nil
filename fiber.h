#pragma once

#ifndef FIBER_H_
#define FIBER_H_

#include <stdbool.h>

#include "object.h"
#include "compiler.h"
#include "bytecode.h"


struct code_pointer {
    objptr_t func;
    struct code *code;
    unsigned int offset;
};


void code_pointer_init_from_func(struct code_pointer*, objptr_t);
void code_pointer_copy(struct code_pointer*, struct code_pointer*);
void code_pointer_terminate(struct code_pointer*);
instr_t code_pointer_get(struct code_pointer*);
objptr_t code_pointer_get_constant(struct code_pointer*, unsigned int);
void code_pointer_jump(struct code_pointer*, unsigned int);
bool code_pointer_is_valid(struct code_pointer*);



struct continuation_frame {
    struct object head;

    objptr_t clink;  // The link to the previous continuation frame
    objptr_t stack_ptr;
    objptr_t environment;
    struct code_pointer instr_pointer;
};




struct fiber {
    // TODO: Prev/Next fiber
    // TODO: Waiting condition
    
    objptr_t clink;  // Continuation / Frame stack
    objptr_t stack_ptr;
    objptr_t environment;
    struct code_pointer instr_pointer;
};


void fiber_init(struct fiber*, objptr_t);
void fiber_terminate(struct fiber*);

void fiber_tick(struct fiber*);


#endif
