// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdlib.h>
#include "mb_tools.h"


/*! \brief Small fixed size data elements management.
 *
 * It is used to management a large number of data elements
 * they with the same memory size, a fixed size.  It allocate
 * a large block for a lot of elements a time for more efficiency
 * utilization of memory.
 *
 * Elements with the size and in a large number usually be accessed
 * very close in time.  Allocate a large block for elements also
 * increase cache hit rate.
 *
 * Blocks are keep track as a linking list.  They are freed when
 * the elmpool_t is freed.  It costs overhead of size of a element
 * for each block.  We use memory of first element of blocks to
 * be next pointer of linking list.  So, it can not be used by user
 * code.
 */
struct _elmpool {
    int elm_sz;
    int inc_num;
    void *frees;
    void *blks;			/* list of allocated blocks. */
};

/*! \brief Create a new data elements pool.
 *
 * elmpool_t provide a pool of fixed size elements to gain better
 * utilization of memory.  It try to allocate bigger memory blocks
 * for multiple elements.
 *
 * \param elm_sz size of elements.
 * \param inc_num is number of elments to allocate every time. (>= 16)
 * \return A elmpool or NULL for error.
 */
elmpool_t *elmpool_new(int elm_sz, int inc_num) {
    int _elm_sz;
    elmpool_t *pool;

    if(inc_num < 16)
	return NULL;

    if(elm_sz >= sizeof(void *))
	_elm_sz = elm_sz;
    else
	_elm_sz = sizeof(void *);

    pool = (elmpool_t *)malloc(sizeof(elmpool_t));
    if(pool == NULL)
	return NULL;

    pool->elm_sz = _elm_sz;
    if(inc_num == 0)
	inc_num = 256;
    pool->inc_num = inc_num;
    pool->frees = NULL;
    pool->blks = NULL;

    return pool;
}

void *elmpool_elm_alloc(elmpool_t *pool) {
    void *blk, *elm;
    int elm_sz, inc_num;
    int i;

    if(pool->frees == NULL) {
	inc_num = pool->inc_num;
	elm_sz = pool->elm_sz;
	blk = malloc(elm_sz * inc_num);
	if(blk == NULL)
	    return NULL;

	*(void **)blk = pool->blks;
	pool->blks = blk;

	blk = blk + elm_sz;
	pool->frees = blk;
	for(i = 2; i < inc_num; i++) {
	    *(void **)blk = blk + elm_sz;
	    blk = *(void **)blk;
	}
	*(void **)blk = NULL;
    }

    elm = pool->frees;
    pool->frees = *(void **)elm;

    return elm;
}

void elmpool_elm_free(elmpool_t *pool, void *elm) {
    *(void **)elm = pool->frees;
    pool->frees = elm;
}

void elmpool_free(elmpool_t *pool) {
    void *blk, *next_blk;

    blk = pool->blks;
    while(blk) {
	next_blk = *(void **)blk;
	free(blk);
	blk = next_blk;
    }
}

#ifdef UNITTEST

#include <CUnit/Basic.h>

void test_elmpool(void) {
    elmpool_t *pool;
    void *elm;
    int i;

    pool = elmpool_new(64, 16);
    for(i = 0; i < 15; i++) {
	elm = elmpool_elm_alloc(pool);
	CU_ASSERT(elm != NULL);
    }
    CU_ASSERT(pool->frees == NULL);

    for(i = 0; i < 15; i++) {
	elm = elmpool_elm_alloc(pool);
	CU_ASSERT(elm != NULL);
    }
    CU_ASSERT(pool->frees == NULL);

    elmpool_elm_free(pool, elm);
    CU_ASSERT(pool->frees == elm);

    elm = elmpool_elm_alloc(pool);
    CU_ASSERT(elm != NULL);
    CU_ASSERT(pool->frees == NULL);

    elmpool_free(pool);
}

CU_pSuite get_tools_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_tools", NULL, NULL);
    CU_ADD_TEST(suite, test_elmpool);

    return suite;
}

#endif /* UNITTEST */
