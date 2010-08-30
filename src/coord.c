/*! \brief Implement coordination tranform mechanism.
 * \file
 * This file implements coordination transforming for containers.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mb_types.h"


#define ASSERT(x)

/* To keep possibility of changing type of aix */
#define MUL(a, b) ((a) * (b))
#define ADD(a, b) ((a) + (b))
#define DIV(a, b) ((a) / (b))
#define SUB(a, b) ((a) - (b))

static void mul_matrix(co_aix *m1, co_aix *m2, co_aix *dst) {
    dst[0] = ADD(MUL(m1[0], m2[0]), MUL(m1[1], m2[3]));
    dst[1] = ADD(MUL(m1[0], m2[1]), MUL(m1[1], m2[4]));
    dst[2] = ADD(ADD(MUL(m1[0], m2[2]), MUL(m1[1], m2[5])), m1[2]);
    dst[3] = ADD(MUL(m1[3], m2[0]), MUL(m1[4], m2[3]));
    dst[4] = ADD(MUL(m1[3], m2[1]), MUL(m1[4], m2[4]));
    dst[5] = ADD(ADD(MUL(m1[3], m2[2]), MUL(m1[4], m2[5])), m1[5]);
}

void matrix_mul(co_aix *m1, co_aix *m2, co_aix *dst) {
    co_aix *_dst = dst;
    co_aix fake_dst[6];
    
    if(m1 == dst || m2 == dst)
	_dst = fake_dst;
    
    mul_matrix(m1, m2, _dst);

    if(m1 == dst || m2 == dst) {
	dst[0] = fake_dst[0];
	dst[1] = fake_dst[1];
	dst[2] = fake_dst[2];
	dst[3] = fake_dst[3];
	dst[4] = fake_dst[4];
	dst[5] = fake_dst[5];
    }
}

void matrix_trans_pos(co_aix *matrix, co_aix *x, co_aix *y) {
    co_aix nx, ny;

    nx = ADD(ADD(MUL(matrix[0], *x),
		 MUL(matrix[1], *y)),
	     matrix[2]);
    ny = ADD(ADD(MUL(matrix[3], *x),
		 MUL(matrix[4], *y)),
	     matrix[5]);
    *x = nx;
    *y = ny;
}

/*! \brief Compute aggregated transform matrix.
 *
 * Base on parent's aggregated matrix if it is existed, or use transform
 * matrix as aggregated matrix. 
 */
static void compute_transform_function(coord_t *visit) {
    if(!coord_is_root(visit))
	mul_matrix(visit->parent->aggr_matrix,
		   visit->matrix, visit->aggr_matrix);
    else
	memcpy(visit->aggr_matrix, visit->matrix, sizeof(visit->matrix));
}

void compute_aggr_of_coord(coord_t *coord) {
    compute_transform_function(coord);
}

/*! \brief Compute aggregated transform matrix for cached coord.
 *
 * \sa \ref img_cache
 */
static void compute_transform_function_cached(coord_t *visit) {
    co_aix *p_matrix;
    co_aix cache_p_matrix[6];
    co_aix cache_scale_x, cache_scale_y;
    
    if(!coord_is_root(visit)) {
	p_matrix = coord_get_aggr_matrix(visit->parent);
	cache_scale_x =
	    sqrtf(p_matrix[0] * p_matrix[0] + p_matrix[3] * p_matrix[3]);
	cache_scale_y =
	    sqrtf(p_matrix[1] * p_matrix[1] + p_matrix[4] * p_matrix[4]);
	cache_p_matrix[0] = cache_scale_x;
	cache_p_matrix[1] = 0;
	cache_p_matrix[2] = 0;
	cache_p_matrix[3] = 0;
	cache_p_matrix[4] = cache_scale_y;
	cache_p_matrix[5] = 0;
	mul_matrix(cache_p_matrix, visit->matrix, visit->aggr_matrix);
    } else {
	memcpy(visit->aggr_matrix, visit->matrix, sizeof(visit->matrix));
    }
}

void compute_aggr_of_cached_coord(coord_t *coord) {
    compute_transform_function_cached(coord);
}

void
compute_aggr(coord_t *coord) {
    if(coord->flags & COF_OWN_CANVAS)
	compute_transform_function_cached(coord);
    else
	compute_transform_function(coord);
}

void compute_reverse(co_aix *orig, co_aix *reverse) {
    co_aix working[6];
    co_aix factor;
    
#define VEC_MAC(src, factor, dst)		\
    do {					\
	(dst)[0] += (src)[0] * (factor);	\
	(dst)[1] += (src)[1] * (factor);	\
	(dst)[2] += (src)[2] * (factor);	\
    } while(0)

    reverse[0] = 1;
    reverse[1] = 0;
    reverse[2] = 0;
    reverse[3] = 0;
    reverse[4] = 1;
    reverse[5] = 0;
    
    memcpy(working, orig, sizeof(co_aix) * 6);

    factor = -working[3] / working[0];
    VEC_MAC(working, factor, working + 3);
    VEC_MAC(reverse, factor, reverse + 3);

    factor = -working[1] / working[4];
    VEC_MAC(working + 3, factor, working);
    VEC_MAC(reverse + 3, factor, reverse);

    reverse[2] = -working[2];
    reverse[5] = -working[5];

    factor = 1 / working[0];
    reverse[0] *= factor;
    reverse[1] *= factor;
    reverse[2] *= factor;
    factor = 1 / working[4];
    reverse[3] *= factor;
    reverse[4] *= factor;
    reverse[5] *= factor;
}

/*! \brief Update aggregate matrices of elements under a sub-tree.
 *
 * A subtree is specified by the root of it.  All elements in the subtree
 * are effected by that changes of matrix of the subtree root.
 *
 * \todo Remove update_aggr_matrix() since it is out of date and
 *	no one use it.
 */
void update_aggr_matrix(coord_t *start) {
    coord_t *visit, *child, *next;

    compute_transform_function(start);

    visit = start;
    while(visit) {
	child = STAILQ_HEAD(visit->children);
	while(child) {
	    compute_transform_function(child);
	    child = STAILQ_NEXT(coord_t, sibling, child);
	}

	if(STAILQ_HEAD(visit->children))
	    visit = STAILQ_HEAD(visit->children);
	else if(STAILQ_NEXT(coord_t, sibling, visit))
	    visit = STAILQ_NEXT(coord_t, sibling, visit);
	else {
	    next = NULL;
	    while(visit->parent && visit->parent != start) {
		visit = visit->parent;
		if(STAILQ_NEXT(coord_t, sibling, visit)) {
		    next = STAILQ_NEXT(coord_t, sibling, visit);
		    break;
		}
	    }
	    visit = next;
	}
    }
}

/*! \brief Initialize a coord object.
 *
 * The object is cleared and matrix was initialized to ID.
 * The object is be a children of specified parent.
 */
void coord_init(coord_t *co, coord_t *parent) {
    memset(co, 0, sizeof(coord_t));
    if(parent) {
	/* insert at tail of children list. */
	co->parent = parent;
	STAILQ_INS_TAIL(parent->children, coord_t, sibling, co);
    }
    mb_obj_init(co, MBO_COORD);
    co->matrix[0] = 1;
    co->matrix[4] = 1;
    co->aggr_matrix[0] = 1;
    co->aggr_matrix[4] = 1;
    co->cur_area = &co->areas[0];
    co->last_area = &co->areas[1];
}

void coord_trans_pos(coord_t *co, co_aix *x, co_aix *y) {
    co_aix nx, ny;

    nx = ADD(ADD(MUL(co->aggr_matrix[0], *x),
		 MUL(co->aggr_matrix[1], *y)),
	     co->aggr_matrix[2]);
    ny = ADD(ADD(MUL(co->aggr_matrix[3], *x),
		 MUL(co->aggr_matrix[4], *y)),
	     co->aggr_matrix[5]);
    *x = nx;
    *y = ny;
}

co_aix coord_trans_size(coord_t *co, co_aix sz) {
    co_aix x, y;

    x = MUL(co->aggr_matrix[0], sz);
    y = MUL(co->aggr_matrix[3], sz);

    return sqrt(x * x + y * y);
}

/*!
 * \note Coords, marked with COF_SKIP_TRIVAL (for temporary), and
 * descendants of them will not be trivaled and the flag with be removed
 * after skipping them.
 */
coord_t *preorder_coord_subtree(coord_t *root, coord_t *last) {
    coord_t *next = NULL;

    ASSERT(last != NULL);
    
    if((!(last->flags & COF_SKIP_TRIVAL)) &&
       STAILQ_HEAD(last->children)) {
	next = STAILQ_HEAD(last->children);
	if(!(next->flags & COF_SKIP_TRIVAL))
	    return next;
    } else {
	next = last;
    }

    do {
	next->flags &= ~COF_SKIP_TRIVAL;
	while(next != root && STAILQ_NEXT(coord_t, sibling, next) == NULL)
	    next = next->parent;
	if(next == root)
	    return NULL;
	next = STAILQ_NEXT(coord_t, sibling, next);
    } while(next->flags & COF_SKIP_TRIVAL);

    return next;
}

coord_t *postorder_coord_subtree(coord_t *root, coord_t *last) {
    coord_t *next;

    if(root == last)
	return NULL;
    
    if(last == NULL) {
	/* Go most left leaf. */
	next = root;
	while(STAILQ_HEAD(next->children))
	    next = STAILQ_HEAD(next->children);
	return next;
    }

    next = last;
    if(STAILQ_NEXT(coord_t, sibling, next) == NULL) /* most right */
	return next->parent;

    /* Go most left leaf of right sibling sub-tree. */
    next = STAILQ_NEXT(coord_t, sibling, next);
    while(STAILQ_HEAD(next->children))
	next = STAILQ_HEAD(next->children);

    return next;
}

#ifdef UNITTEST

#include <CUnit/Basic.h>

void test_update_aggr_matrix(void) {
    coord_t elms[6];
    co_aix x, y;

    coord_init(elms, NULL);
    coord_init(elms + 1, elms);
    coord_init(elms + 2, elms);
    coord_init(elms + 3, elms + 1);
    coord_init(elms + 4, elms + 1);
    coord_init(elms + 5, elms + 2);

    /* | 2 -1 0 |
     * | 0  1 0 |
     * | 0  0 1 |
     */
    elms[0].matrix[0] = 2;
    elms[0].matrix[1] = -1;

    /* | 1 3 0 |
     * | 5 1 0 |
     * | 0 0 1 |
     */
    elms[1].matrix[1] = 3;
    elms[1].matrix[3] = 5;

    update_aggr_matrix(elms);

    /* | -3 5 0 |
     * | 5  1 0 | 
     * | 0  0 1 |
     */
    CU_ASSERT(elms[3].aggr_matrix[0] == -3);
    CU_ASSERT(elms[3].aggr_matrix[1] == 5);
    CU_ASSERT(elms[3].aggr_matrix[2] == 0);
    CU_ASSERT(elms[3].aggr_matrix[3] == 5);
    CU_ASSERT(elms[3].aggr_matrix[4] == 1);
    CU_ASSERT(elms[3].aggr_matrix[5] == 0);

    CU_ASSERT(elms[4].aggr_matrix[0] == -3);
    CU_ASSERT(elms[4].aggr_matrix[1] == 5);
    CU_ASSERT(elms[4].aggr_matrix[2] == 0);
    CU_ASSERT(elms[4].aggr_matrix[3] == 5);
    CU_ASSERT(elms[4].aggr_matrix[4] == 1);
    CU_ASSERT(elms[4].aggr_matrix[5] == 0);

    CU_ASSERT(elms[5].aggr_matrix[0] == 2);
    CU_ASSERT(elms[5].aggr_matrix[1] == -1);
    CU_ASSERT(elms[5].aggr_matrix[2] == 0);
    CU_ASSERT(elms[5].aggr_matrix[3] == 0);
    CU_ASSERT(elms[5].aggr_matrix[4] == 1);
    CU_ASSERT(elms[5].aggr_matrix[5] == 0);

    x = 50;
    y = 99;
    coord_trans_pos(elms + 5, &x, &y);
    CU_ASSERT(x == 1);
    CU_ASSERT(y == 99);
}

void test_preorder_coord_subtree(void) {
    coord_t elms[6];
    coord_t *last;

    coord_init(elms, NULL);
    coord_init(elms + 1, elms);
    coord_init(elms + 2, elms);
    coord_init(elms + 3, elms + 1);
    coord_init(elms + 4, elms + 1);
    coord_init(elms + 5, elms + 2);

    last = elms;
    last = preorder_coord_subtree(elms, last);
    CU_ASSERT(last == elms + 1);
    last = preorder_coord_subtree(elms, last);
    CU_ASSERT(last == elms + 3);
    last = preorder_coord_subtree(elms, last);
    CU_ASSERT(last == elms + 4);
    last = preorder_coord_subtree(elms, last);
    CU_ASSERT(last == elms + 2);
    last = preorder_coord_subtree(elms, last);
    CU_ASSERT(last == elms + 5);
    last = preorder_coord_subtree(elms, last);
    CU_ASSERT(last == NULL);
}

CU_pSuite get_coord_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_coord", NULL, NULL);
    CU_ADD_TEST(suite, test_update_aggr_matrix);
    CU_ADD_TEST(suite, test_preorder_coord_subtree);

    return suite;
}

#endif
