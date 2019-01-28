#pragma once

#ifndef CLOSURE_H_
#define CLOSURE_H_


#include "object.h"


struct code {
    objptr_t constant_vector;
    unsigned int code_size;
    unsigned int code_alloc;
    unsigned int *codes;
};


struct closure {
    struct object head;
    bool native;
    
    objptr_t parameter_list;
    struct code code;
    objptr_t environment;
    
    // TODO: Field for native code
};


extern struct object_type TYPE_CLOSURE;


// TODO

#endif
