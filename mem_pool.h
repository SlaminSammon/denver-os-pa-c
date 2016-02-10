/*
 * Created by Ivo Georgiev on 2/9/16.
 */

#ifndef DENVER_OS_PA_C_MEM_POOL_H
#define DENVER_OS_PA_C_MEM_POOL_H

#include <stddef.h>

/* type declarations */

typedef struct _pool {
    char *pool;
    size_t total_size;
    size_t alloc_size;
    unsigned num_allocs;
    unsigned num_gaps;
} pool_t, *pool_pt;

typedef struct _alloc {
    size_t size;
    char *mem;
} alloc_t, *alloc_pt;

typedef enum _alloc_status { ALLOC_OK, ALLOC_FAIL } alloc_status;

typedef enum _alloc_policy { FIRST_FIT, BEST_FIT } alloc_policy;

/* function declarations */

/* TODO descriptions */
pool_pt mem_pool_open(size_t size, alloc_policy policy);

alloc_status mem_pool_close(pool_pt pool);

alloc_pt mem_new_alloc(pool_pt pool, size_t size);

alloc_status mem_del_alloc(pool_pt pool, alloc_pt alloc);

#endif //DENVER_OS_PA_C_MEM_POOL_H
