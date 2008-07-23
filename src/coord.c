#include <stdio.h>
#include <string.h>

typedef struct coord {
    int seq;
    float matrix[6];
    float aggr_matrix[6];
    struct coord *parent;
    struct coord *children, *sibling;
} coord_t;

static void mul_matrix(float *m1, float *m2, float *dst) {
    dst[0] = m1[0] * m2[0] + m1[1] * m2[3];
    dst[1] = m1[0] * m2[1] + m1[1] * m2[4];
    dst[2] = m1[0] * m2[2] + m1[1] * m2[5] + m1[2];
    dst[3] = m1[3] * m2[0] + m1[4] * m2[3];
    dst[4] = m1[3] * m2[1] + m1[4] * m2[4];
    dst[5] = m1[3] * m2[2] + m1[4] * m2[5] + m1[5];
}

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

#ifdef UNITTEST

#include <CUnit/Basic.h>

void test_update_aggr_matrix(void) {
    coord_t elms[6];

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
}

CU_pSuite get_coord_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_coord", NULL, NULL);
    CU_ADD_TEST(suite, test_update_aggr_matrix);

    return suite;
}

#endif
