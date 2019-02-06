#pragma once

#ifndef CHARACTER_H_
#define CHARACTER_H_


#include "object.h"


typedef unsigned int unichar_t;


struct character {
    struct object head;
    unichar_t code;
};


extern struct object_type TYPE_CHARACTER;


objptr_t get_character(unichar_t code);
unichar_t character_value(objptr_t);

void init_characters();
void terminate_characters();


#endif
