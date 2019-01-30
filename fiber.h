#pragma once

#ifndef FIBER_H_
#define FIBER_H_

#include "object.h"
#include "compiler.h"


struct code_pointer {
    // TODO: Pointer to lambda to avoid accidental garbage collection?
    struct code *code;
    unsigned int offset;
};


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
};


#endif
