/*! \brief Implement coordination tranform mechanism.
 * \file
 * This file implements coordination transforming for containers.
 */
#include <stdio.h>
#include <string.h>

typedef float co_aix;
/*! \brief A coordination system.
 *
 * It have a transform function defined by matrix to transform
 * coordination from source space to target space.
 * Source space is where the contained is drawed, and target space
 * is where the coordination of parent container of the element
 * represented by this coord object.
 */
typedef struct coord {
    int seq;
    co_aix matrix[6];
    co_aix aggr_matrix[6];
    struct coord *parent;
    struct coord *children, *sibling;
} coord_t;

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

/*! \brief Compute agrregated transform function.
 *
 * Base on parent's aggregated matrix if it is existed, or use transform
 * matrix as aggregated matrix. 
 */
static void compute_transform_function(coord_t *visit) {
    if(visit->parent)
	mul_matrix(visit->parent->aggr_matrix,
		   visit->matrix, visit->aggr_matrix);
    else
	memcpy(visit->aggr_matrix, visit->matrix, sizeof(visit->matrix));
}

/*! \brief Update aggregate matrices of elements under a sub-tree.
 *
 * A subtree is specified by the root of it.  All elements in the subtree
 * are effected by that changes of matrix of the subtree root.
 */
void update_aggr_matrix(coord_t *start) {
    coord_t *visit, *child, *next;

    compute_transform_function(start);

    visit = start;
    while(visit) {
	child = visit->children;
	while(child) {
	    compute_transform_function(child);
	    child = child->sibling;
	}

	if(visit->children)
	    visit = visit->children;
	else if(visit->sibling)
	    visit = visit->sibling;
	else {
	    next = NULL;
	    while(visit->parent && visit->parent != start) {
		visit = visit->parent;
		if(visit->sibling) {
		    next = visit->sibling;
		    break;
		}
	    }
	    visit = next;
	}
    }
}

void coord_init(coord_t *co, coord_t *parent) {
    memset(co, 0, sizeof(coord_t));
    if(parent) {
	co->parent = parent;
	co->sibling = parent->children;
	parent->children = co;
    }
    co->matrix[0] = 1;
    co->matrix[4] = 1;
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

CU_pSuite get_coord_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_coord", NULL, NULL);
    CU_ADD_TEST(suite, test_update_aggr_matrix);

    return suite;
}

#endif
