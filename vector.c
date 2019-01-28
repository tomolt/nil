#include <stdlib.h>

#include "character.h"

#include "vector.h"



void init_vector(struct vector *vector)
{
    vector->is_string = false;
    vector->member_count = 0;
    vector->member_alloc = 0;
    vector->data = NULL;
}


void terminate_vector(struct vector *vector)
{
    unsigned int i;
      
    if (vector->data != NULL) {
	for (i = 0; i < vector->member_count; i++) {
	    decrease_refcount(vector->data[i]);
	}
	
	free(vector->data);
	vector->data = NULL;
    }

    vector->member_count = 0;
    vector->member_alloc = 0;
}


unsigned int vector_slot_count(struct vector *vector)
{
    return vector->member_count;
}


objptr_t vector_slot_accessor(struct vector *vector, unsigned int slot)
{
    if (slot >= vector->member_count) {
	return EMPTY_LIST;
    } else {
	return vector->data[slot];
    }
}


bool vector_eqv(struct vector *v1,
		struct vector *v2,
		enum eqv_strictness strictness)
{
    unsigned int i;

    switch (strictness) {
    case EQV_STRICT: return v1 == v2;
    case EQ_STRICT:
	return (v1 == v2) || (v1->member_count == 0 && v2->member_count == 0);
    case EQUAL_STRICT:
	if ((v1->member_count != v2->member_count)
	    || (v1->is_string != v2->is_string)) {
	    return false;
	}

	for (i = 0; i < v1->member_count; i++)
	{
	    if (!eqv(v1->data[i], v2->data[i], strictness)) {
		return false;
	    }
	}
    
	return true;
	
    default: return false;
    }
}


DEFTYPE(TYPE_VECTOR,
	struct vector,
	init_vector,
	terminate_vector,
	vector_slot_count,
	vector_slot_accessor,
	vector_eqv);



objptr_t make_vector(objptr_t fill, unsigned int size)
{
    objptr_t vector;
    unsigned int i;

    vector = object_allocate(&TYPE_VECTOR);
    if (vector == EMPTY_LIST) return vector;

    for (i = 0; i < size; i++)
    {
	vector_set(vector, i, fill);
    }

    return vector;
}


objptr_t make_string(objptr_t fill, unsigned int size)
{
    objptr_t vector;

    vector = make_vector(EMPTY_LIST, 0);
    if (vector == EMPTY_LIST) return vector;

    ((struct vector*) dereference(vector))->is_string = true;
    return vector;
}


objptr_t make_string_from_c_string(const char *str)
{
    objptr_t string;
    unsigned int i;

    string = make_string(EMPTY_LIST, 0);
    if (string == EMPTY_LIST) return string;

    for (i = 0; str[i] != '\0'; i++)
    {
	vector_append(string, get_character((unichar_t) str[i]));
    }
    
    return string;
}


objptr_t vector_copy(objptr_t ptr)
{
    objptr_t copy;
    struct vector *original;
    unsigned int i;

    if (is_of_type(ptr, &TYPE_VECTOR)) {
	original = (struct vector*) dereference(ptr);
	
	if (original->is_string) {
	    copy = make_string(EMPTY_LIST, 0);
	} else {
	    copy = make_vector(EMPTY_LIST, 0);
	}

	if (copy == EMPTY_LIST) return copy;

	for (i = 0; i < original->member_count; i++)
	{
	    vector_append(copy, original->data[i]);
	}

	return copy;
    } else {
	return EMPTY_LIST;
    }
}


objptr_t vector_get(objptr_t ptr, unsigned int index)
{
    struct vector *vector;

    if (is_of_type(ptr, &TYPE_VECTOR)) {
	vector = (struct vector*) dereference(ptr);
	if (index >= vector->member_count || vector->data == NULL) {
	    return EMPTY_LIST;  // TODO: Error?
	} else {
	    return vector->data[index];
	}
    } else {
	return EMPTY_LIST;
    }
}


void vector_set(objptr_t ptr, unsigned int index, objptr_t value)
{
    unsigned int i;
    unsigned int growth;
    struct vector *vector;

    if (is_of_type(ptr, &TYPE_VECTOR)) {
	vector = (struct vector*) dereference(ptr);
	
	if (index >= vector->member_alloc) {
	    /*
	     * Vector doesn't contain enough free slots, we have
	     * to (re)allocate.
	     */
	    if (vector->member_alloc == 0) {
		// Initial size is 8
		growth = 8;
	    } else {
		// Multiply size by 2
		growth = vector->member_alloc;
	    }

	    if ((vector->member_alloc + growth) < index) {
		growth = (index - vector->member_alloc) + 16;
	    }
	    
	    vector->data = realloc(vector->data,
				   (vector->member_alloc + growth) * sizeof(objptr_t));

	    // FIXME: Handle out of memory!

	    for (i = 0; i < growth; i++)
	    {
		if (vector->is_string) {
		    vector->data[vector->member_alloc + i] = get_character('\0');
		} else {
		    vector->data[vector->member_alloc + i] = EMPTY_LIST;
		}
	    }
	    
	    vector->member_alloc += growth;
	}

	decrease_refcount(vector->data[index]);
	vector->data[index] = value;
	increase_refcount(value);
	
	if (index >= vector->member_count) {
	    vector->member_count = index + 1;
	}
    }
}


unsigned int vector_length(objptr_t ptr)
{
    struct vector *vector;

    if (is_of_type(ptr, &TYPE_VECTOR)) {
	vector = (struct vector*) dereference(ptr);
	return vector->member_count;
    } else {
	return 0;
    }
}


void vector_append(objptr_t ptr, objptr_t value)
{
    vector_set(ptr, vector_length(ptr), value);
}


void vector_insert(objptr_t ptr, unsigned int index, objptr_t value)
{
    unsigned int i;
    unsigned int length;

    /*
     * TODO: This is inefficient as hell!
     */

    length = vector_length(ptr);
    
    for (i = length - 1; i >= index; i--)
    {
	vector_set(ptr, i + 1, vector_get(ptr, i));
    }
    
    vector_set(ptr, index, value);
}


void vector_remove(objptr_t ptr, unsigned int index)
{
    unsigned int i;
    struct vector *vector;
    
    if (is_of_type(ptr, &TYPE_VECTOR)) {
	vector = (struct vector*) dereference(ptr);
	
	if (index >= vector->member_count
	    || vector->data == NULL) {
	    return;
	}

	decrease_refcount(vector->data[index]);
	for (i = index; i < vector->member_count - 1; i++)
	{
	    vector->data[i] = vector->data[i + 1];
	}
	vector->data[vector->member_count - 1] = EMPTY_LIST;
	vector->member_count--;
	
	/*
	 * TODO: Release memory if allocated block is to big?
	 */
    }
}



void init_vectors()
{
}


void terminate_vectors()
{
    free_type_instances(&TYPE_VECTOR);
}
