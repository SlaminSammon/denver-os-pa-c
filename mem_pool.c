/*
 * Created by Ivo Georgiev on 2/9/16.
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h> // for perror()

#include "mem_pool.h"

/* Constants */
static const float      MEM_FILL_FACTOR                 = 0.75;
static const unsigned   MEM_EXPAND_FACTOR               = 2;

static const unsigned   MEM_POOL_STORE_INIT_CAPACITY    = 20;
static const float      MEM_POOL_STORE_FILL_FACTOR      = MEM_FILL_FACTOR;
static const unsigned   MEM_POOL_STORE_EXPAND_FACTOR    = MEM_EXPAND_FACTOR;

static const unsigned   MEM_NODE_HEAP_INIT_CAPACITY     = 40;
static const float      MEM_NODE_HEAP_FILL_FACTOR       = MEM_FILL_FACTOR;
static const unsigned   MEM_NODE_HEAP_EXPAND_FACTOR     = MEM_EXPAND_FACTOR;

static const unsigned   MEM_GAP_IX_INIT_CAPACITY        = 40;
static const float      MEM_GAP_IX_FILL_FACTOR          = MEM_FILL_FACTOR;
static const unsigned   MEM_GAP_IX_EXPAND_FACTOR        = MEM_EXPAND_FACTOR;

/* Type declarations */
typedef struct _node {
    alloc_t alloc_record;
    unsigned used;
    unsigned allocated;
    struct _node *next, *prev; // doubly-linked list for gap deletion
} node_t, *node_pt;

typedef struct _gap {
    size_t size;
    node_pt node;
} gap_t, *gap_pt;

typedef struct _pool_mgr {
    pool_t pool;
    node_pt node_heap;
    unsigned total_nodes;
    unsigned used_nodes;
    gap_pt gap_ix;
} pool_mgr_t, *pool_mgr_pt;


/* Static global variables */
static pool_mgr_pt *pool_store = NULL; // an array of pointers, only expand
static unsigned pool_store_size = 0;
static unsigned pool_store_capacity = 0;


/* Forward declarations of static functions */
static alloc_status _mem_resize_pool_store();
static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr);
static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr);
static alloc_status
        _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                           size_t size,
                           node_pt node);
static alloc_status
        _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                                size_t size,
                                node_pt node);
static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr);


/* Definitions of user-facing functions */
alloc_status mem_init() {
    // TODO implement

	//If the pool store already has been initialized
	//Return the allocation status stating that it already has been initialized.
	if (pool_store != NULL){
		return ALL0C_CALLED_AGAIN;
	}
	//Allocate room for the initial amount pool store capacity
	pool_store = (pool_mgr_pt *)calloc(MEM_POOL_STORE_INIT_CAPACITY, sizeof(pool_mgr_t); 
	//If our allocation went correctly
	if (pool_store != NULL){
		//Set the size and capacity of the pool store
		pool_store_size = MEM_POOL_STORE_INIT_CAPACITY;
		pool_store_capacity = 0;

		return ALLOC_OK;
	}
    //If we get to this point then we know an allocation failed
	return ALLOC_FAIL;
}

alloc_status mem_free() {
    // TODO implement

    return ALLOC_FAIL;
}

pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    // If the array of pool stores hasn't been allocated then allocate it.
	if (pool_store == NULL){
		//if the memory fails to allocate then return NULL.
		if (mem_init() != ALLOC_FAIL){
			return NULL;
		}
	}

	//CHeck to see if we have the maximum amount of pool_stores or not.
	if (_mem_resize_pool_store() != ALLOC_OK){
		return NULL;//IF it fails then return NULL.
	}

	pool_store_capacity++;//Increase the amount of pools in the pool_store.

	pool_mgr_pt manager = calloc(1, sizeof(pool_mgr_t));//Create a new pool.
	if (manager == NULL){
		return NULL;//If allocation fails yadda yadda ya.
	}

	pool_store[pool_store_capacity - 1] = manager;//Place the new pool in the next open place of the pool_store array.
	//Set pools values
	(*manager).pool.policy = policy;
	(*manager).pool.alloc_size = size;
	(*manager).pool.mem = malloc(size);

	if ((*manager).mem == NULL){
		free((*manager).pool.mem);//delete the memory allocation
		free(manager);//delete the allocation of the pool store.
		//Restore these states to their pre function states.
		pool_store[pool_store_capacity - 1] = NULL;
		pool_store_capacity--;
		return NULL;
	}

	//Allocate the node heap and gap index
	(*manager).gap_ix = calloc(MEM_GAP_IX_INIT_CAPACITY, sizeof(gap_t));
	(*manager).node_heap = calloc(MEM_NODE_HEAP_INIT_CAPACITY, sizeof(gap_t));
	if ((*manager), node_heap == NULL || (*manager).gap_ix == NULL){
		//Free all allocated memory
		free((*manager).node_heap);
		free((*manager).gap_ix);
		free((*manager).pool.mem);
		free(manager);
		//Restore these states to their pre function states.
		pool_store[pool_store_capacity - 1] = NULL;
		pool_store_capacity--;
		return NULL;
	}
	//Initialize all gap and node members.
	(*manager).total_nodes = MEM_NODE_HEAP_INIT_CAPACITY;
	(*manager).used_nodes = 0;

	(*manager).total_gaps = MEM_NODE_GAP_IX_CAPACITY;
	(*manager).used_gaps = 0;

    return (pool_pt) (*manager).pool;
}

alloc_status mem_pool_close(pool_pt pool) {
    // TODO implement
	pool_mgr_pt manager;

	//Go through the pool store array if two pools match then thay
	//pool_mgr will become our target for deletion
	unsigned int bool = 0, i = 0;
	for (i = 0; i < pool_store_capacity){
		if ((*pool_store[i]).pool == (*pool)){
			manager = pool_store[i];
			//Check to see if this is the last element in the array.
			if (i == pool_store_capacity - 1){
				bool = 1;//Set this for later use.
			}
		}
	}
	//If the pool was not found then we cannot deallocate
	if (manager == NULL){
		return ALLOC_NOT_FREED;
	}
	//If the pool was in the last element of the array
	if (bool == 1){
		//Set the last element to NULL
		pool_store[pool_store_capacity - 1] = NULL;
	}
	else{
		pool_store[i] = pool_store[pool_store_capacity - 1];
		pool_store[pool_store_capacity - 1] = NULL
	}
	//free all allocated memory
	free((*manager).pool.mem);
	free((*manager).node_heap);
	free((*manager).gap_ix);
	free(manager);
	pool_store_capacity--;

    return ALLOC_OK;
}

alloc_pt mem_new_alloc(pool_pt pool, size_t size) {
    // TODO implement

    return NULL;
}

alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc) {
    // TODO implement

    return ALLOC_FAIL;
}

// NOTE: Allocates a dynamic array. Caller responsible for releasing.
void mem_inspect_pool(pool_pt pool, pool_segment_pt segments, unsigned *num_segments) {
    // TODO implement
}


/* Definitions of static functions */
static alloc_status _mem_resize_pool_store() {
    // TODO implement

    return ALLOC_FAIL;
}

static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) {
    // TODO implement

    return ALLOC_FAIL;
}

static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) {
    // TODO implement

    return ALLOC_FAIL;
}

static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                                       size_t size,
                                       node_pt node) {
    // TODO implement

    return ALLOC_FAIL;
}

static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                                            size_t size,
                                            node_pt node) {
    // TODO implement

    return ALLOC_FAIL;
}


static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr) {

    // TODO implement

    return ALLOC_FAIL;
}


