/*! \brief Determine who should be re-drawed.
 * \file
 * When part of graphic are chagned, not mater size, shape, or position,
 * the components effected or overlaid should be re-drawed.  This module
 * figures out components that should be re-drawed.
 */
#include <stdio.h>
#include "mb_types.h"

struct _geo {
    co_aix x, y;
    co_aix w, h;
    co_aix rel_x, rel_y;
    co_aix orig_x, orig_y;
    int n_subs;
    struct _geo *subs;
};

static int is_scale_overlay(co_aix x1, co_aix w1, co_aix x2, co_aix w2) {
    if(x1 > x2) {
	if((x1 - x2) >= w2)
	    return 0;
    } else {
	if((x2 - x1) >= w1)
	    return 0;
    }
    return 1;
}

static int is_overlay(geo_t *r1, geo_t *r2) {
    if(!is_scale_overlay(r1->x, r1->w, r2->x, r2->w))
	return 0;
    if(!is_scale_overlay(r1->y, r1->h, r2->y, r2->h))
	return 0;

    return 1;
}

void geo_init(geo_t *g, int n_pos, co_aix pos[][2]) {
    co_aix min_x, max_x;
    co_aix min_y, max_y;
    co_aix x, y;
    int i;

    min_x = max_x = pos[0][0];
    min_y = max_y = pos[0][1];
    for(i = 1; i < n_pos; i++) {
	x = pos[i][0];
	if(x < min_x)
	    min_x = x;
	else if(x > max_x)
	    max_x = x;
	y = pos[i][1];
	if(y < min_y)
	    min_y = y;
	else if(y > max_y)
	    max_y = y;
    }
    g->x = min_x;
    g->w = max_x - min_x;
    g->y = min_y;
    g->h = max_y - min_y;
}

void geo_mark_overlay(geo_t *g, int n_others, geo_t **others,
		      int *n_overlays, geo_t **overlays) {
    int i, ov_idx;

    ov_idx = 0;
    for(i = 0; i < n_others; i++) {
	if(is_overlay(g, others[i]))
	    overlays[ov_idx++] = others[i];
    }
    *n_overlays = ov_idx;
}


#ifdef UNITTEST

#include <CUnit/Basic.h>

void test_geo_init(void) {
    co_aix data[][2] = {
	{33, 25}, {49, 12},
	{14, 28}, {39, 56}};
    geo_t g;
    
    geo_init(&g, 4, data);
    CU_ASSERT(g.x == 14);
    CU_ASSERT(g.w == 35);
    CU_ASSERT(g.y == 12);
    CU_ASSERT(g.h == 44);
}

void test_geo_mark_overlay(void) {
    geo_t _geos[3], *geos[3], *overlays[3];
    geo_t g;
    int i, n_ov;

    for(i = 0; i < 3; i++) {
	_geos[i].x = i * 50;
	_geos[i].y = i * 50;
	_geos[i].w = 55;
	_geos[i].h = 66;
	geos[i] = _geos + i;
    }
    g.x = 88;
    g.y = 79;
    g.w = 70;
    g.h = 70;

    /* overlay with geos[1] and geos[2] */
    geo_mark_overlay(&g, 3, geos, &n_ov, overlays);
    CU_ASSERT(n_ov == 2);
    CU_ASSERT(overlays[0] == geos[1]);
    CU_ASSERT(overlays[1] == geos[2]);

    /* right side of geos[1], and up side of geos[2] */
    g.x = 105;
    g.y = 50;
    g.w = 50;
    g.h = 51;
    geo_mark_overlay(&g, 3, geos, &n_ov, overlays);
    CU_ASSERT(n_ov == 1);
    CU_ASSERT(overlays[0] == geos[2]);
}

CU_pSuite get_geo_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_geo", NULL, NULL);
    CU_ADD_TEST(suite, test_geo_init);
    CU_ADD_TEST(suite, test_geo_mark_overlay);

    return suite;
}

#endif
