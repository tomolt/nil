#pragma once

#ifndef OBJECT_H_
#define OBJECT_H_


#include <stdint.h>
#include <stdbool.h>



#define MAX_TYPE_BLOCK_BUFFER_SIZE_KB 1000



struct object;
typedef unsigned int objptr_t;

extern objptr_t EMPTY_LIST, NIL_TRUE, NIL_FALSE;


enum eqv_strictness {
    EQV_STRICT,
    EQ_STRICT,
    EQUAL_STRICT
};


struct object_type_allocation_block {
    struct object_type_allocation_block *next;
};

struct object_type {

    /*
     * Function interface
     *
     * Functions are constant to avoid overriding
     */

    const unsigned int size;
    
    // The following two functions are used only for
    // internal purposes. The actual allocation is
    // done by the heap array manager.
    void (*initialize_instance)(struct object*);
    void (*terminate_instance)(struct object*);

    // Slot access functions. The actual slot access
    // is done by the heap array manager.
    unsigned int (*get_slot_count)(struct object*);
    objptr_t (*get_slot)(struct object*, unsigned int);

    // Comparison functions
    bool (*eqv)(struct object*, struct object*, enum eqv_strictness);


    /*
     * Block allocator part
     *
     * Variables are mutable
     */

    // Total block count can be calculated
    // by adding active to buffered
    unsigned long active_block_count;
    unsigned long buffered_block_count;
    unsigned long deleted_block_count;

    struct object_type_allocation_block *blocks;
};


#define OBJECT_REFCOUNT_BITMASK  0x00ff  /* These bits have to be the lowest bits! */
#define OBJECT_MARK_FLAG_BITMASK 0x0100

struct object {
    uint16_t flags;
    struct object_type *type;
} __attribute__ ((packed));


#define DEFTYPE(NAME, STRUCTURE_NAME, INIT, TERMINATE, SLOT_COUNT, GET_SLOT, EQV) \
    struct object_type NAME = {						\
	sizeof(STRUCTURE_NAME),						\
	(void (*)(struct object*)) INIT,				\
	(void (*)(struct object*)) TERMINATE,				\
	(unsigned int (*)(struct object*)) SLOT_COUNT,			\
	(objptr_t (*)(struct object*, unsigned int)) GET_SLOT,		\
	(bool (*)(struct object*, struct object*, enum eqv_strictness)) EQV, \
	0, 0, 0, NULL							\
    }


/*
 * Memory manager part
 */

// Memory access functions
objptr_t object_allocate(struct object_type*);
struct object *dereference(objptr_t);

// Type and equality functions
bool is_of_type(objptr_t, struct object_type*);
bool eqv(objptr_t, objptr_t, enum eqv_strictness strict);

// Slot accessors
unsigned int slot_count(objptr_t);
objptr_t get_slot(objptr_t, unsigned int);

// Garbage collector functions
void declare_root_object(objptr_t);
void make_refcount_immune(objptr_t);

void increase_refcount(objptr_t);
void decrease_refcount(objptr_t);

// Init/Termination functions
void free_type_instances(struct object_type*);

void init_memory_system();
void end_memory_system();


#endif
