#include <stdlib.h>

#include "character.h"


void init_character(struct character *character)
{
    character->code = 0;
}


void terminate_character(struct character *character)
{
}


bool character_eqv(struct character *c1,
		   struct character *c2,
		   enum eqv_strictness strictness)
{
    return c1->code == c2->code;
}


DEFTYPE(TYPE_CHARACTER,
	struct character,
	init_character,
	terminate_character,
	NULL,
	NULL,
	character_eqv);



#define FIXED_CHARACTER_COUNT 256
static objptr_t FIXED_CHARACTERS[FIXED_CHARACTER_COUNT];


objptr_t get_character(unichar_t code)
{
    objptr_t character;
    struct character *instance;

    character = EMPTY_LIST;
    
    if (code >= 0 && code < FIXED_CHARACTER_COUNT) {
	if (FIXED_CHARACTERS[code] != EMPTY_LIST) {
	    return FIXED_CHARACTERS[code];
	}
    }

    character = object_allocate(&TYPE_CHARACTER);
    
    if (character != EMPTY_LIST) {
	instance = (struct character*) dereference(character);
	instance->code = code;
	
	if(code >= 0 && code < FIXED_CHARACTER_COUNT) {
	    declare_root_object(character);
	    make_refcount_immune(character);
	    FIXED_CHARACTERS[code] = character;
	}
    }

    return character;
}


unichar_t character_value(objptr_t c)
{
    struct character *inst;
    
    if (is_of_type(c, &TYPE_CHARACTER)) {
        inst = (struct character*) dereference(c);
        return inst->code;
    } else {
        return 0;
    }
}


void init_characters()
{
    unsigned int i;

    for (i = 0; i < FIXED_CHARACTER_COUNT; i++)
    {
	FIXED_CHARACTERS[i] = EMPTY_LIST;
	make_refcount_immune(FIXED_CHARACTERS[i]);
    }
}


void terminate_characters()
{
    free_type_instances(&TYPE_CHARACTER);
}
