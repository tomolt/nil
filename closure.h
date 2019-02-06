#pragma once

#ifndef CLOSURE_H_
#define CLOSURE_H_


#include "compiler.h"

#include "object.h"


struct closure_prototype {
    struct object head;

    bool is_macro;
    
    objptr_t parameter_vector;
    objptr_t rest_parameter;
    
    struct code code;
};


struct closure {
    struct object head;

    objptr_t prototype;
    objptr_t environment;
};


extern struct object_type TYPE_CLOSURE_PROTOTYPE;
extern struct object_type TYPE_CLOSURE;

objptr_t make_closure_prototype(objptr_t);
objptr_t make_closure_from_prototype(objptr_t, objptr_t);

#endif
