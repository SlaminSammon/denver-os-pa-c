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
    unsigned gap_ix_capacity;
    unsigned gap_ix_size;
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

/*
* Function Name: mem_init
* Passed Variables: None
* Return Type: alloc_status
* Purpose: To intialize the pool store. The pool store is an array,
* of all the pool managers being used within the program.
* This array is the size of the variable MEM_POOL_STORE_INIT_CAPACITY
* which is 20, giving us space for 20 pool_managers. In case this function
* is called when the pool store has already been intialized, thre is a check to
* not reintialize the pool_store. If the allocation fails then the function
* returns ALLOC_FAIL.
*/
alloc_status mem_init() {
    // TODO implement

	//If the pool store already has been initialized
	//Return the allocation status stating that it already has been initialized.
	if (pool_store != NULL){
		return ALL0C_CALLED_AGAIN;
	}
	//Allocate room for the initial amount pool store capacity
	pool_store = (pool_mgr_pt *)calloc(MEM_POOL_STORE_INIT_CAPACITY, sizeof(pool_mgr_t));
	//If our allocation went correctly
	if (pool_store != NULL){
		//Set the size and capacity of the pool store
		pool_store_size = MEM_POOL_STORE_INIT_CAPACITY;

		return ALLOC_OK;
	}
    //If we get to this point then we know an allocation failed
	return ALLOC_FAIL;
}

alloc_status mem_free() {
    // TODO implement
	/* If mem_init hasn't been called yell at things. */
	if (pool_store == NULL){
		return ALL0C_CALLED_AGAIN;
	}
	/* for all initialized pool managers */
	for (unsigned int i = 0; i < pool_store_capacity; ++i){
		/* delete the memory of the poolmgr */
		mem_pool_close(&pool_store[i]->pool);
	}
	/* free the memory allocated */
	free(pool_store);
	if (pool_store != NULL){
		/* If the deallocation failed return a failure */
		return ALLOC_FAIL;
	}
	/* reset static variables */
	pool_store_capacity = 0;
	pool_store_size = 0;
	return ALLOC_OK;
}

/*
 * Function Name: mem_pool_open
 * Passed Variables: size_t size, alloc_policy policy
 * Return Type: pool_pt
 * Purpose: This function creates a new pool of memory of the passed size.
 * This is put into a new pool_mgr that has all of it's default values set.
 * The pool's default values are also set. These default values are set using
 * constant value specified at the start of the file.
 */
pool_pt mem_pool_open(size_t size, alloc_policy policy) {
    // If the array of pool stores hasn't been allocated then allocate it.
	if (pool_store == NULL){
		//if the memory fails to allocate then return NULL.
		if (mem_init() == ALLOC_FAIL){
			return NULL;
		}
	}

	//CHeck to see if we have the maximum amount of pool_stores or not.
	if (_mem_resize_pool_store() != ALLOC_OK){
		return NULL;//IF it fails then return NULL.
	}

	pool_store_capacity++;//Increase the amount of pools in the pool_store.
    int bool = 0;
    pool_mgr_pt manager = NULL;
    /* Loop until allocation succeeds */
    while(bool == 0){
        manager = calloc(1, sizeof(pool_mgr_t));//Create a new pool.
        if (manager != NULL){
            /* If the allocation succeded escape the loop */
            bool = 1;
        }
    }

	pool_store[pool_store_capacity - 1] = manager;//Place the new pool in the next open place of the pool_store array.
	//Set pools values
	(*manager).pool.policy = policy;
	(*manager).pool.total_size = size;
	(*manager).pool.mem = malloc(size);

	if ((*manager).pool.mem == NULL){
		free((*manager).pool.mem);//delete the memory allocation
		free(manager);//delete the allocation of the pool store.
		//Restore these states to their pre function states.
		pool_store[pool_store_capacity - 1] = NULL;
		pool_store_capacity--;
		return NULL;
	}

	//Allocate the node heap and gap index
	(*manager).gap_ix = calloc(MEM_GAP_IX_INIT_CAPACITY, sizeof(gap_t));
	(*manager).node_heap = calloc(MEM_NODE_HEAP_INIT_CAPACITY, sizeof(node_t));
	if ((*manager).node_heap == NULL || (*manager).gap_ix == NULL){
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
	(*manager).used_nodes = 1;
    (*manager).gap_ix_capacity = 1;
    //call add to gap ix here once written for a gap the size of the pool
    (*manager).gap_ix_size = MEM_GAP_IX_INIT_CAPACITY;
    (*manager).node_heap[0].alloc_record.size = size;
    (*manager).node_heap[0].allocated = 0;
    (*manager).node_heap[0].used = 1;
    (*manager).node_heap[0].prev = NULL;
    if(_mem_add_to_gap_ix(manager, size, &(*manager).node_heap[0]) == ALLOC_FAIL){
        printf("Failed to add first node to gap index.");
        exit(0);
    }


    return (pool_pt) manager;
}

/*
 * Function Name: mem_pool_close
 * Passed Variables: pool_pt pool
 * Return Type: alloc_status
 * Purpose: This function deletes a pool from memory. Alongside the pool
 * all allocated memory is deleted and the pool's manager is removed
 * from the pool store array. If the pool is not aember of any of the pool
 * managers then the function returns ALLOC_NOT_FREED telling the program that
 * the pool was not deallocated.
 */
alloc_status mem_pool_close(pool_pt pool) {
    // TODO implement
    pool_mgr_pt manager= NULL;

	//Go through the pool store array if two pools match then thay
	//pool_mgr will become our target for deletion
	unsigned int bool = 0, i = 0;
    for (i = 0; i < pool_store_capacity; ++i){
		if (&(*pool_store[i]).pool == pool){
			manager = pool_store[i];
			//Check to see if this is the last element in the array.
			if (i == pool_store_capacity - 1){
				bool = 1;//Set this for later use.
			}
		}
	}
	//If the pool was not found then we cannot deallocate
	if (manager == NULL){
		return ALLOC_FAIL;
	}
	//If the pool was in the last element of the array
	if (bool == 1){
		//Set the last element to NULL
		pool_store[pool_store_capacity - 1] = NULL;
	}
	else{
        /* Swap pool stores, set end node to null. */
		pool_store[i] = pool_store[pool_store_capacity - 1];
        pool_store[pool_store_capacity - 1] = NULL;
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
    /* Upcast the pool to access the manager */
    size_t remainSpace = 0;
    const pool_mgr_pt manager = (pool_mgr_pt) pool;
    /* If any of these cases are true then exit */
    if((*manager).gap_ix_capacity == 0 || _mem_resize_node_heap(manager) == ALLOC_FAIL ||
       (*manager).total_nodes <= (*manager).used_nodes){
        exit(0);
    }
    node_pt newNode = NULL;
    if(manager->pool.policy == BEST_FIT){}
    /* First Fit allocation */
    if(manager->pool.policy == FIRST_FIT){
        for (unsigned int i = 0; i<(*manager).total_nodes; ++i){
            /* Find the first empty node in the array. Needs to be able to fit the size we're allocating */
            if((*manager).node_heap[i].allocated == 0 && (*manager).node_heap[i].alloc_record.size >= size){
                newNode = &(*manager).node_heap[i];//Set the new node to the found gap.
                remainSpace = newNode->alloc_record.size - size;//Place the remaining amount of memory into a holder for later
                break;
            }
        }
    }
    /* if the node couldn't be allocated return null */
    if(newNode == NULL){
        return NULL;
    }
    /* remove the node from the gap index */
    if(_mem_remove_from_gap_ix(manager,size,newNode) != ALLOC_OK){
        return NULL;
    }
    manager->pool.num_allocs++;//Change the amount of allocations to the pool
    manager->pool.alloc_size += size;
    /* Alter the nodes values */
    newNode->used = 1;
    newNode->allocated = 1;
    newNode->alloc_record.size = size;
    /* Check if we need a new node for the next gap or if we don't need a new gap. */
    if(_mem_resize_node_heap(manager)== ALLOC_FAIL && remainSpace != 0){
        exit(0);
    }
    if(remainSpace != 0){
        
    }
    /****** Start Back Here ******/
    /*Add the gap that may have been created back t the gap index.
    
    return NULL;
}

alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc) {
    // TODO implement

    return ALLOC_FAIL;
}

// NOTE: Allocates a dynamic array. Caller responsible for releasing.
void mem_inspect_pool(pool_pt pool, pool_segment_pt *segments, unsigned *num_segments) {
    // TODO implement
    /* Upcast the pool to a pool manager */
    const pool_mgr_pt manager = (pool_mgr_pt) pool;
    /* Allocate the segments array */
    pool_segment_pt segs = calloc((*manager).used_nodes, sizeof(pool_segment_t));
    if(segments == NULL){
        return;
    }
    int place = 0; // THis is a placeholder for the segments array since it's only the used nodes.
    for(unsigned int i = 0; i < (*manager).total_nodes;++i){
        /*Skip all unused nodes */
        if((*manager).node_heap[i].used == 0){
            continue;//Moves to the next iteration
        }
        /* Fill the segments array */
        segs[place].size = (*manager).node_heap[i].alloc_record.size;
        segs[place].allocated = (*manager).node_heap[i].allocated;
    }
    /*pass these values back */
    *num_segments = (*manager).total_nodes;
    *segments = segs;
    return;
    
}


/* Definitions of static functions */
static alloc_status _mem_resize_pool_store() {
    // TODO implement
    /* Check to see if we have too many pools. */
    if(pool_store_size > pool_store_capacity * MEM_POOL_STORE_FILL_FACTOR){
        /* Create a new pool manager that is a reallocated 'pool_store' */
        pool_mgr_pt* reallocated_store = (pool_mgr_pt *) realloc(pool_store, pool_store_capacity* MEM_POOL_STORE_EXPAND_FACTOR * sizeof(pool_mgr_pt));
        if(reallocated_store == NULL){
            /* If the allocation failed then we return a fail state. */
            return ALLOC_FAIL;
        }
        else{
            /* Set the pool_store to the newly allocated pool_store 'reallocated_store*/
            pool_store = reallocated_store;
        }
        return ALLOC_OK;
    }
    /* If we don't have too many pools then we can return that everything is okay */
    else{
        return ALLOC_OK;
    }
}

static alloc_status _mem_resize_node_heap(pool_mgr_pt pool_mgr) {
    // TODO implement
    /* Check to see if we have too many nodes */
    if((*pool_mgr).used_nodes > (*pool_mgr).total_nodes * MEM_NODE_HEAP_FILL_FACTOR){
        /* Create a new node_pt that is a reallocated node heap. */
        /* We use the expand factor to increase the size. This is simply multiplying by 2. */
        node_pt reallocated_node = (node_pt) realloc((*pool_mgr).node_heap, (*pool_mgr).total_nodes * MEM_NODE_HEAP_EXPAND_FACTOR * sizeof(node_pt));
        if(reallocated_node == NULL){
            /* If the allocation failed then return ALLOC_FAIL. */
            return ALLOC_FAIL;
        }
        else{
            /* Set the node heap to the newly allocated 'reallocated_node' */
            (*pool_mgr).node_heap = reallocated_node;
            (*pool_mgr).total_nodes *= MEM_NODE_HEAP_EXPAND_FACTOR;
            return ALLOC_OK;
        }
    }
    /* If we are okay on nodes then return okay. */
    else{
        return ALLOC_OK;
    }
}

static alloc_status _mem_resize_gap_ix(pool_mgr_pt pool_mgr) {
    // TODO implement

    if((*pool_mgr).used_nodes > (*pool_mgr).total_nodes * MEM_NODE_HEAP_FILL_FACTOR){
        /* Create a new node_pt that is a reallocated gap index. */
        /* We use the expand factor to increase the size. This is simply multiplying by 2. */
        gap_pt reallocated_gap = (gap_pt) realloc((*pool_mgr).gap_ix, (*pool_mgr).gap_ix_size * MEM_GAP_IX_EXPAND_FACTOR * sizeof(gap_pt));
        if(reallocated_gap == NULL){
            /* If the allocation failed then return ALLOC_FAIL. */
            return ALLOC_FAIL;
        }
        else{
            /* Set the node heap to the newly allocated 'reallocated_gap' */
            (*pool_mgr).gap_ix = reallocated_gap;
            (*pool_mgr).gap_ix_capacity *= MEM_GAP_IX_EXPAND_FACTOR;
            return ALLOC_OK;
        }
    }
    /* If we are okay on gaps then return okay. */
    else{
        return ALLOC_OK;
    }
}

static alloc_status _mem_add_to_gap_ix(pool_mgr_pt pool_mgr,
                                       size_t size,
                                       node_pt node) {
    // TODO implement
    /* Check to see if we need to resize */
    if(pool_mgr->gap_ix_size / pool_mgr->gap_ix_capacity > MEM_GAP_IX_FILL_FACTOR){
        if(_mem_resize_gap_ix(pool_mgr) == ALLOC_FAIL){
            return ALLOC_FAIL;
        }
    }
    /* Set the nodes values */
    (*node).allocated = 0;
    (*node).used = 1;
    /* Add the gap to the index and node heap */
    (*pool_mgr).gap_ix[pool_mgr->gap_ix_capacity].node = node;
    (*pool_mgr).gap_ix[pool_mgr->gap_ix_capacity].size = size;
    /*Increase the amount of gaps */
    (*pool_mgr).gap_ix_capacity++;
    /* Sort the index */
    return _mem_sort_gap_ix(pool_mgr);

}

static alloc_status _mem_remove_from_gap_ix(pool_mgr_pt pool_mgr,
                                            size_t size,
                                            node_pt node) {
    // TODO implement

    return ALLOC_FAIL;
}


static alloc_status _mem_sort_gap_ix(pool_mgr_pt pool_mgr) {
    /* This is just a bubble sort that sorts the gaps based on their size. */
    for(unsigned int i =0; i < (*pool_mgr).gap_ix_capacity-1; ++i){
        for(unsigned int j = 0; j < (*pool_mgr).gap_ix_capacity-2;++j){
            /* If the sizes are unordered. */
            if((*pool_mgr).gap_ix[j].size > (*pool_mgr).gap_ix[j+1].size){
                /* Swap them :D */
                gap_t swapper = (*pool_mgr).gap_ix[j];
                (*pool_mgr).gap_ix[j] = (*pool_mgr).gap_ix[i];
                (*pool_mgr).gap_ix[i] = swapper;
            }
        }
    }

    return ALLOC_OK;
}


