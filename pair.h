#pragma once

#ifndef NUMBERS_H_
#define NUMBERS_H_

#include <stdlib.h>

#include "object.h"


struct pair {
    struct object head;
    objptr_t car;
    objptr_t cdr;
};


extern struct object_type TYPE_PAIR;



objptr_t get_car(objptr_t);
objptr_t get_cdr(objptr_t);

void set_car(objptr_t, objptr_t);
void set_cdr(objptr_t, objptr_t);

objptr_t cons(objptr_t, objptr_t);


#endif
