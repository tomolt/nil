
#include <assert.h>
#include <stdlib.h>

#include "fiber.h"

#include "object.h"




/*
 * OBJECT/TYPE BOOKKEEPING
 */


void free_type_instances(struct object_type *type)
{
    struct object_type_allocation_block *next;
    
    assert(type != NULL);
    
    while (type->blocks != NULL) {
	next = type->blocks->next;
	free(type->blocks);
	type->blocks = next;
    }
}


static struct object *INTERN_object_allocate_instance(struct object_type *type)
{
    struct object_type_allocation_block *block;
    struct object *instance;
    
    assert(type != NULL);

    instance = NULL;
    
    if (type->blocks != NULL) {
	block = type->blocks;
	type->blocks = block->next;
	type->buffered_block_count--;
	type->active_block_count++;
	instance = (struct object*) block;
    } else {
	instance = malloc(type->size);
	if (instance != NULL) {
	    type->active_block_count++;
	}
    }
    
    if (instance != NULL) {
	instance->flags = 0x0000;
	instance->type = type;

	if (type->initialize_instance != NULL) {
	    type->initialize_instance(instance);
	}
    }
    
    return instance;
}


static void INTERN_object_free_instance(struct object *object)
{
    struct object_type *type;
    struct object_type_allocation_block *block;
    
    assert((object != NULL) && (object->type != NULL));

    type = object->type;

    if (type->terminate_instance != NULL) {
	type->terminate_instance(object);
    }

    /*
     * Decide whether to buffer or to free
     * by the amount of times the block buffer
     * was exhausted.
     *
     * Idea: keep track of the amount of calls to free(),
     *       the higher it gets, the more objects are
     *       allowed in the block buffer.
     */
    if ((type->buffered_block_count < type->deleted_block_count)
	&& ((type->buffered_block_count * type->size) < (MAX_TYPE_BLOCK_BUFFER_SIZE_KB * 1000))) {
	block = (struct object_type_allocation_block*) object;
	block->next = type->blocks;
	type->blocks = block;
	type->active_block_count--;
	type->buffered_block_count++;
    } else {
	free(object);
	type->active_block_count--;
	type->deleted_block_count++;
    }
}



static void INTERN_mark_object(struct object *object)
{
    if (object != NULL) {
	object->flags |= OBJECT_MARK_FLAG_BITMASK;
    }
}


static void INTERN_unmark_object(struct object *object)
{
    if (object != NULL) {
	object->flags = object->flags & (~OBJECT_MARK_FLAG_BITMASK);
    }
}


static bool INTERN_object_is_marked(struct object *object)
{
    assert(object != NULL);
    return (object->flags & OBJECT_MARK_FLAG_BITMASK) != 0;
}



static unsigned int INTERN_object_slot_count(struct object *object)
{
    assert((object != NULL) && (object->type != NULL));
    if ((object->type->get_slot_count == NULL)
	|| (object->type->get_slot == NULL)) {
	return 0;
    } else {
	return object->type->get_slot_count(object);
    }
}


static objptr_t INTERN_object_get_slot(struct object *object, unsigned int slot)
{
    assert((object != NULL) && (object->type != NULL));

    if (object->type->get_slot != NULL) {
	return object->type->get_slot(object, slot);
    } else {
	return EMPTY_LIST;
    }
}





/*
 * HEAP-BASED MEMORY MANAGER
 */

#define HEAP_CELL_FLAG_FREE 0x01

struct heap_cell {
    uint8_t flags;
    union {
	struct object *object;
	objptr_t next;
    } value;
} __attribute__ ((packed));

static unsigned long HEAP_ARRAY_SLOT_COUNT = 0;
static unsigned long HEAP_ARRAY_USED_SLOT_COUNT = 0;
static struct heap_cell *HEAP_ARRAY = NULL;
objptr_t HEAP_ARRAY_FREELIST;
bool GLOBAL_REFCOUNT_LOCK = false;

objptr_t EMPTY_LIST, NIL_TRUE, NIL_FALSE;



static objptr_t heap_array_address_to_objptr(struct heap_cell *addr)
{
    return (objptr_t) (addr - HEAP_ARRAY);
}

static struct heap_cell *dereference_slot(objptr_t ptr)
{
    assert(ptr >= 0 && ptr < HEAP_ARRAY_SLOT_COUNT);
    return &(HEAP_ARRAY[ptr]);
}


static void add_to_freelist(struct heap_cell *slot)
{
    assert(slot != NULL);
    slot->flags |= HEAP_CELL_FLAG_FREE;
    slot->value.next = HEAP_ARRAY_FREELIST;
    HEAP_ARRAY_FREELIST = heap_array_address_to_objptr(slot);
    HEAP_ARRAY_USED_SLOT_COUNT--;
}


static void grow_heap_array(unsigned int slot_delta)
{
    objptr_t i;

    if (slot_delta > 0) {
	/*
	 * Grow the array
	 */
	HEAP_ARRAY = realloc(HEAP_ARRAY, (HEAP_ARRAY_SLOT_COUNT + slot_delta) * sizeof(struct heap_cell));

	/*
	 * Add new slots to heap array free list.
	 * Note: Since EMPTY_LIST has to be set before
	 * the first heap array can be allocated, we skip
	 * it with this ugly condition in the initializer
	 * of the "for" loop. Since the objptr_t of the
	 * empty list is always 0, we are not allowed to
	 * add it to the freelist if we encounter it.
	 * We only encounter it when HEAP_ARRAY_SLOT_COUNT
	 * is zero, therefore we have to set i to 1.
	 */
	for (i = ((HEAP_ARRAY_SLOT_COUNT == 0)? 1 : 0); i < slot_delta; i++)
	{
	    add_to_freelist(&(HEAP_ARRAY[HEAP_ARRAY_SLOT_COUNT + i]));
	}

	/*
	 * Increase declared array size
	 */
	HEAP_ARRAY_SLOT_COUNT += slot_delta;
    }
}


static struct heap_cell *find_fresh_heap_array_slot()
{
    struct heap_cell *slot;

    if (HEAP_ARRAY_FREELIST == EMPTY_LIST) {
	/*
	 * Grow the heap.
	 */
	if (HEAP_ARRAY_SLOT_COUNT < 1024) {
	    grow_heap_array(1024);
	} else {
	    grow_heap_array((unsigned int) (HEAP_ARRAY_SLOT_COUNT * 0.5));
	}
    }

    /*
     * XXX: Check whether freelist still is EMPTY_LIST
     */
    
    /*
     * Fetch slot and advance freelist
     */
    slot = dereference_slot(HEAP_ARRAY_FREELIST);
    if (HEAP_ARRAY_FREELIST != EMPTY_LIST && slot != NULL) {
	HEAP_ARRAY_FREELIST = slot->value.next;
    }

    /*
     * Initialize slot
     */
    if (slot != NULL) {
	slot->flags &= ~HEAP_CELL_FLAG_FREE;
	slot->value.object = NULL;
	HEAP_ARRAY_USED_SLOT_COUNT++;
    }
    
    return slot;
}


static void object_deallocate(objptr_t ptr)
{
    struct heap_cell *slot;

    slot = dereference_slot(ptr);
    
    if ((slot == NULL) || ((slot->flags & HEAP_CELL_FLAG_FREE) != 0)) {
	return;
    }
    
    if (slot->value.object != NULL) {
	INTERN_object_free_instance(slot->value.object);
    }
    
    add_to_freelist(slot);
}




/*
 * Public interface
 */


struct object *dereference(objptr_t ptr)
{
    // TODO: Bounds check! --> return EMPTY_LIST
    if ((HEAP_ARRAY[ptr].flags & HEAP_CELL_FLAG_FREE) != 0) {
	return NULL;
    } else {
	return HEAP_ARRAY[ptr].value.object;
    }
}


bool is_of_type(objptr_t ptr, struct object_type *type)
{
    struct object *object;

    object = dereference(ptr);
    if (object != NULL) {
	return object->type == type;
    } else {
	return false;
    }
}


bool eqv(objptr_t p1, objptr_t p2, enum eqv_strictness strictness)
{
    struct object *o1;
    struct object *o2;

    if (p1 == p2) return true;
    
    o1 = dereference(p1);
    o2 = dereference(p2);

    if ((o1 == NULL || o2 == NULL)) {
	// Objects are probably either "()", "#t" or "#f",
	// but the objptr_t's are not equal, therefore
	// the objects are not equal.
	return false;
    }

    if (o1 == o2) {
	// Objects are equal
	return true;
    }

    if (o1->type == NULL || o2->type == NULL) {
	// Objects with no type can't be compared
	return false;
    }

    if (o1->type != o2->type) {
	// Objects are not of the same type, so
	// they can't be equal
	return false;
    }

    if (o1->type->eqv == NULL) {
	// Since the type pointers are pointing to
	// the same object we don't have to check
	// for o2's type->eqv pointer
	// TODO: Maybe iterate over the slots and
	//       check their equality?
	return false;
    } else {
	return o1->type->eqv(o1, o2, strictness);
    }
}


objptr_t object_allocate(struct object_type *type)
{
    struct heap_cell *slot;

    assert(type != NULL);
    
    slot = find_fresh_heap_array_slot();

    if (slot != NULL) {
	slot->value.object = INTERN_object_allocate_instance(type);
	return heap_array_address_to_objptr(slot);
    } else {
	return EMPTY_LIST;
    }
}


void increase_refcount(objptr_t ptr)
{
    struct heap_cell *slot;
    struct object *object;

    if (GLOBAL_REFCOUNT_LOCK
	|| ptr == EMPTY_LIST
	|| ptr == NIL_TRUE
	|| ptr == NIL_FALSE) {
	return;
    }
    
    slot = dereference_slot(ptr);
    assert((slot != NULL) && ((slot->flags & HEAP_CELL_FLAG_FREE) == 0));

    object = slot->value.object;
    if (object == NULL) return;
    
    if ((object->flags & OBJECT_REFCOUNT_BITMASK) != OBJECT_REFCOUNT_BITMASK) {
        object->flags = (object->flags & ~OBJECT_REFCOUNT_BITMASK)
            | (((object->flags & OBJECT_REFCOUNT_BITMASK) + 1)
               & OBJECT_REFCOUNT_BITMASK);
    }
}


void decrease_refcount(objptr_t ptr)
{
    struct heap_cell *slot;
    struct object *object;


    if (ptr == EMPTY_LIST
	|| ptr == NIL_TRUE
	|| ptr == NIL_FALSE) {
	return;
    }
    
    slot = dereference_slot(ptr);
    assert((slot != NULL) && ((slot->flags & HEAP_CELL_FLAG_FREE) == 0));

    object = slot->value.object;
    if (object == NULL) return;
    
    // Make sure that the reference count doesn't exceed the maximum number, or zero
    if (((object->flags & OBJECT_REFCOUNT_BITMASK) != 0) &&
	((object->flags & OBJECT_REFCOUNT_BITMASK) != OBJECT_REFCOUNT_BITMASK)) {
        object->flags = (object->flags & ~OBJECT_REFCOUNT_BITMASK)
            | (((object->flags & OBJECT_REFCOUNT_BITMASK) - 1)
               & OBJECT_REFCOUNT_BITMASK);
    }

    if (((object->flags & OBJECT_REFCOUNT_BITMASK) == 0)
        && !GLOBAL_REFCOUNT_LOCK) {
	object_deallocate(ptr);
    }
}




/*
 * GARBAGE COLLECTOR
 */


static void mark_object(objptr_t ptr)
{
    unsigned int index;
    unsigned int count;
    struct object *object;
    
    /*
     * Get the object
     */
    object = dereference(ptr);

    /*
     * Check whether the object has to be marked and do so if needed
     */
    if ((object == NULL)
	|| INTERN_object_is_marked(object)) {
	return;
    } else {
	INTERN_mark_object(object);
    }
    
    /*
     * Loop over the object's slots and mark them.
     * TODO: Use an external stack for this to avoid a
     *       native return stack overflow!
     */
    count = INTERN_object_slot_count(object);
    
    for (index = 0; index < count; index++)
    {
	mark_object(INTERN_object_get_slot(object, index));
    }
}



static unsigned int ROOT_OBJECT_POOL_SIZE = 0;
static unsigned int ROOT_OBJECT_POOL_ALLOC_SIZE = 0;
static objptr_t *ROOT_OBJECT_POOL = NULL;


void declare_root_object(objptr_t ptr)
{
    /*
     * Add the pointer to a dynamic array of root objects.
     */
    
    if (ROOT_OBJECT_POOL_SIZE >= ROOT_OBJECT_POOL_ALLOC_SIZE) {
	ROOT_OBJECT_POOL_ALLOC_SIZE += 128;
	ROOT_OBJECT_POOL = realloc(ROOT_OBJECT_POOL,
				   ROOT_OBJECT_POOL_ALLOC_SIZE * sizeof(objptr_t));
    }

    ROOT_OBJECT_POOL[ROOT_OBJECT_POOL_SIZE] = ptr;
    ROOT_OBJECT_POOL_SIZE++;
}


void make_refcount_immune(objptr_t ptr)
{
    struct object *object;

    object = dereference(ptr);

    if (object != NULL) {
	object->flags |= OBJECT_REFCOUNT_BITMASK;
    }
}



extern struct fiber *FIBER_LIST;

static void mark()
{
    unsigned int index;
    struct fiber *fib, *end;
    
    /*
     * Mark root objects
     */
    for (index = 0; index < ROOT_OBJECT_POOL_SIZE; index++)
    {
	mark_object(ROOT_OBJECT_POOL[index]);
    }

    /*
     * Mark fibers
     */
    if (FIBER_LIST != NULL) {
        fib = FIBER_LIST;
        end = fib;
        do {
            mark_object(fib->self);
            fib = fib->next;
        } while (fib != NULL && fib != end);
    }
}


static void sweep()
{
    unsigned long current_slot;
    bool refcount_lock_keeper;

    /*
     * This garbage collector doesn't rebuild the freelist,
     * it detects cells which are not in the freelist and
     * adds them to the front of the freelist if they contain
     * unmarked objects. NULLs are considered to be part of
     * the freelist.
     */

    // We set the refcount lock to avoid accidental refcount garbage collection
    refcount_lock_keeper = GLOBAL_REFCOUNT_LOCK;
    GLOBAL_REFCOUNT_LOCK = true;
    
    for (current_slot = 0;
	 current_slot < HEAP_ARRAY_SLOT_COUNT;
	 current_slot++)
    {
	if (((HEAP_ARRAY[current_slot].flags & HEAP_CELL_FLAG_FREE) != 0)
	    || (HEAP_ARRAY[current_slot].value.object == NULL)) {
	    /*
	     * The current slot is either NULL or a freelist element.
	     * Therefore, we can leave it unaffected.
	     */
	    continue;
	}
	
	/*
	 * We can now assume that the current slot points to an actual
	 * object in memory.
	 */
	if (INTERN_object_is_marked(HEAP_ARRAY[current_slot].value.object)) {
	    /*
	     * The object is referenced, we remove the mark and leave
	     * it in memory.
	     */
	    INTERN_unmark_object(HEAP_ARRAY[current_slot].value.object);
	} else {
	    /*
	     * The object is not referenced anymore, delete it!
	     */
	    object_deallocate(heap_array_address_to_objptr(&(HEAP_ARRAY[current_slot])));
	}
    }

    GLOBAL_REFCOUNT_LOCK = refcount_lock_keeper;
}


static void garbage_collect()
{
    mark();
    sweep();
}


void maybe_garbage_collect()
{
    static unsigned int counter = 0;
    
    // TODO: Find better GC invocation criteria
    if (counter++ > 1000000) {
        counter = 0;
        garbage_collect();
    }
}



/*
 * INIT SECTION
 */


void init_memory_system()
{
    ROOT_OBJECT_POOL = malloc(128 * sizeof(objptr_t));

    EMPTY_LIST = 0;
    HEAP_ARRAY_FREELIST = EMPTY_LIST;
    declare_root_object(EMPTY_LIST);
    
    grow_heap_array(1024);  // Initialize heap by growing it

    
    /*
     * We're now allocating the memory locations for the
     * "special" objects #t and #f.
     */
    
    NIL_TRUE = heap_array_address_to_objptr(find_fresh_heap_array_slot());
    declare_root_object(NIL_TRUE);
    
    NIL_FALSE = heap_array_address_to_objptr(find_fresh_heap_array_slot());
    declare_root_object(NIL_FALSE);
}


void end_memory_system()
{
    /*
     * Call the garbage collector without marking to collect all
     * active objects.
     */
    // Avoid decreasing refcount of already sweeped objects
    GLOBAL_REFCOUNT_LOCK = true;
    sweep();

    
    /*
     * Finally, delete the heap and related structures.
     */
    
    if (ROOT_OBJECT_POOL != NULL) {
	free(ROOT_OBJECT_POOL);
    }
    
    if (HEAP_ARRAY != NULL) {
	HEAP_ARRAY_SLOT_COUNT = 0;
	HEAP_ARRAY_FREELIST = EMPTY_LIST;
	free(HEAP_ARRAY);
    }
}
