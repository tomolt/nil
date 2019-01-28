#include <stdlib.h>

#include "environment.h"


void init_environment(struct environment *environment)
{
    unsigned int i;
    
    environment->parent = EMPTY_LIST;
    
    for (i = 0; i < ENVIRONMENT_SLOT_COUNT; i++)
    {
	environment->slots[i].key = EMPTY_LIST;
	environment->slots[i].value = EMPTY_LIST;
    }

    environment->extended_slot_count = 0;
    environment->extended_slot_alloc = 0;
    environment->extended_slots = NULL;
}


void terminate_environment(struct environment *environment)
{
    unsigned int i;

    decrease_refcount(environment->parent);
    environment->parent = EMPTY_LIST;

    for (i = 0; i < ENVIRONMENT_SLOT_COUNT; i++)
    {
	decrease_refcount(environment->slots[i].key);
	environment->slots[i].key = EMPTY_LIST;
	decrease_refcount(environment->slots[i].value);
	environment->slots[i].value = EMPTY_LIST;
    }

    if (environment->extended_slots != NULL) {
	for (i = 0; i < environment->extended_slot_count; i++)
	{
	    decrease_refcount(environment->extended_slots[i].key);
	    environment->extended_slots[i].key = EMPTY_LIST;
	    decrease_refcount(environment->extended_slots[i].value);
	    environment->extended_slots[i].value = EMPTY_LIST;
	}
	free(environment->extended_slots);
    }
}


unsigned int environment_slot_count(struct environment *environment)
{
    return ((ENVIRONMENT_SLOT_COUNT + environment->extended_slot_count) * 2) + 1;
}


objptr_t environment_slot_accessor(struct environment *environment, unsigned int slot)
{
    if (slot == 0) {
	return environment->parent;
    } else if (slot < (2 * ENVIRONMENT_SLOT_COUNT) + 1) {
	slot--;
	if (slot % 2 == 0) {
	    return environment->slots[slot / 2].key;
	} else {
	    return environment->slots[slot / 2].value;
	}
    } else if (slot < (2 * (ENVIRONMENT_SLOT_COUNT + environment->extended_slot_count)) + 1) {
	slot = slot - 2 * ENVIRONMENT_SLOT_COUNT - 1;
	if (environment->extended_slots == NULL) {
	    return EMPTY_LIST;
	} else 	if (slot % 2 == 0) {
	    return environment->extended_slots[slot / 2].key;
	} else {
	    return environment->extended_slots[slot / 2].value;
	}
    } else {
	return EMPTY_LIST;
    }
}


bool environment_eqv(struct environment *e1,
		     struct environment *e2,
		     enum eqv_strictness strictness)
{
    // Environments can't be compared recursively!
    return e1 == e2;
}


DEFTYPE(TYPE_ENVIRONMENT,
	struct environment,
	init_environment,
	terminate_environment,
	environment_slot_count,
	environment_slot_accessor,
	environment_eqv);



objptr_t environment_get_parent(objptr_t ptr)
{
    struct environment *environment;

    if (is_of_type(ptr, &TYPE_ENVIRONMENT)) {
	environment = (struct environment*) dereference(ptr);
	return environment->parent;
    } else {
	return EMPTY_LIST;
    }
}


void environment_set_parent(objptr_t ptr, objptr_t parent)
{
    struct environment *environment;

    if (is_of_type(ptr, &TYPE_ENVIRONMENT)) {
	environment = (struct environment*) dereference(ptr);
	decrease_refcount(environment->parent);
	environment->parent = parent;
	increase_refcount(parent);
    }
}



static objptr_t *INTERNAL_environment_get_binding(objptr_t ptr,
						  objptr_t variable)
{
    unsigned int i;
    struct environment *environment;

    /*
     * I know, GOTOs are bad, but in this case they prevent
     * us from filling the C stack by recursing up the tree.
     */
    
restart:
    if (eqv(ptr, EMPTY_LIST, EQV_STRICT)) {
	return NULL;
    } else if (is_of_type(ptr, &TYPE_ENVIRONMENT)) {
	environment = (struct environment*) dereference(ptr);
	
	for (i = 0; i < ENVIRONMENT_SLOT_COUNT; i++)
	{
	    if (eqv(environment->slots[i].key,
		    variable,
		    EQV_STRICT)) {
		return &(environment->slots[i].value);
	    }
	}

	if (environment->extended_slots != NULL) {
	    for (i = 0; i < environment->extended_slot_count; i++)
	    {
		if (eqv(environment->slots[i].key,
			variable,
			EQV_STRICT)) {
		    return &(environment->extended_slots[i].value);
		}
	    }
	}

	/*
	 * The current environment does not contain the binding,
	 * so we go to the parent environment.
	 */
	ptr = environment->parent;
	goto restart;
    } else {
	return NULL;
    }
}


objptr_t environment_get_binding(objptr_t ptr, objptr_t variable)
{
    objptr_t *binding;

    binding = INTERNAL_environment_get_binding(ptr, variable);
    if (binding == NULL) {
	return EMPTY_LIST;
    } else {
	return *binding;
    }
}


void environment_bind(objptr_t ptr, objptr_t variable, objptr_t value)
{
    unsigned int i;
    objptr_t *binding;
    struct environment *environment;

    if (!is_of_type(ptr, &TYPE_ENVIRONMENT)) return;  // TODO: Error?
    
    environment = (struct environment*) dereference(ptr);
    binding = INTERNAL_environment_get_binding(ptr, variable);
    
    if (binding == NULL) {
	/* Binding does not exist yet, add to current environment */
	if (environment->extended_slots == NULL) {
	    /*
	     * No extended bindings present yet, so we're trying to
	     * add to the "fixed" slots
	     */
	    for (i = 0; i < ENVIRONMENT_SLOT_COUNT; i++)
	    {
		if (eqv(environment->slots[i].key,
			EMPTY_LIST,
			EQV_STRICT)) {
		    /*
		     * Slots are initialized with EMPTY_LIST keys,
		     * so we have found a nice spot ;-)
		     */
		    environment->slots[i].key = variable;
		    increase_refcount(variable);
		    environment->slots[i].value = value;
		    increase_refcount(value);
		    return;
		}
	    }
	}

	/*
	 * No free internal bindings, so we have to allocate
	 * the variables dynamically.
	 */
	if (environment->extended_slot_count >= environment->extended_slot_alloc
	    || environment->extended_slots == NULL) {
	    /*
	     * The dynamic space is exhausted, so we grow.
	     * FIXME: Handle realloc() failures!
	     */
	    environment->extended_slots =
		realloc(environment->extended_slots,
			environment->extended_slot_alloc + 16 * sizeof(struct environment_slot));
	    environment->extended_slot_alloc += 16;
	}

	environment->extended_slots[environment->extended_slot_count].key = variable;
	increase_refcount(variable);
	environment->extended_slots[environment->extended_slot_count].value = value;
	increase_refcount(value);
	environment->extended_slot_count++;
    } else {
	/* Replace an existing environment */
	decrease_refcount(*binding);
	*binding = value;
	increase_refcount(value);
    }
}
