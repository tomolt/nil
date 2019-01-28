#pragma once

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

#include "object.h"


#define ENVIRONMENT_SLOT_COUNT 16


struct environment_slot {
    objptr_t key;
    objptr_t value;
};


struct environment {
    struct object head;
    
    objptr_t parent;
    
    struct environment_slot slots[ENVIRONMENT_SLOT_COUNT];

    unsigned short extended_slot_count;
    unsigned short extended_slot_alloc;
    struct environment_slot *extended_slots;
};


extern struct object_type ENVIRONMENT_TYPE;


objptr_t environment_get_parent(objptr_t);
void environment_set_parent(objptr_t, objptr_t);
objptr_t environment_get_binding(objptr_t, objptr_t);
void environment_bind(objptr_t, objptr_t, objptr_t);


#endif
