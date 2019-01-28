#pragma once

#ifndef VECTOR_H_
#define VECTOR_H_

#include "object.h"


struct vector {
    struct object head;
    bool is_string;
    unsigned int member_count;
    unsigned int member_alloc;
    objptr_t *data;
};


extern struct object_type TYPE_VECTOR;


objptr_t make_vector(objptr_t, unsigned int);
objptr_t make_string(objptr_t, unsigned int);
objptr_t make_string_from_c_string(const char*);
objptr_t vector_copy(objptr_t);

objptr_t vector_get(objptr_t, unsigned int);
void vector_set(objptr_t, unsigned int, objptr_t);
unsigned int vector_length(objptr_t);
void vector_append(objptr_t, objptr_t);
void vector_insert(objptr_t, unsigned int, objptr_t);
void vector_remove(objptr_t, unsigned int);


void init_vectors();
void terminate_vectors();


#endif
