#pragma once

#ifndef CLOSURE_H_
#define CLOSURE_H_


#include "compiler.h"

#include "object.h"


struct closure {
    struct object head;
    
    objptr_t parameter_list;
    struct code code;
    objptr_t environment;
};


extern struct object_type TYPE_CLOSURE;

// TODO

#endif
