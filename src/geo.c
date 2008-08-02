/*! \brief Determine who should be re-drawed.
 * \file
 * When part of graphic are chagned, not mater size, shape, or position,
 * the components effected or overlaid should be re-drawed.  This module
 * figures out components that should be re-drawed.
 */
#include <stdio.h>
#include <string.h>
#include "mb_types.h"

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

static int _is_overlay(area_t *r1, area_t *r2) {
    if(!is_scale_overlay(r1->x, r1->w, r2->x, r2->w))
	return 0;
    if(!is_scale_overlay(r1->y, r1->h, r2->y, r2->h))
	return 0;

    return 1;
}

int is_overlay(area_t *r1, area_t *r2) {
    return _is_overlay(r1, r2);
}

void area_init(area_t *area, int n_pos, co_aix pos[][2]) {
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

    area->x = min_x;
    area->w = max_x - min_x + 1;
    area->y = min_y;
    area->h = max_y - min_y + 1;
}

void geo_init(geo_t *g) {
    memset(g, 0, sizeof(geo_t));
    g->cur_area = g->areas;
    g->last_area = g->areas + 1;
}

void geo_from_positions(geo_t *g, int n_pos, co_aix pos[][2]) {
    area_init(g->cur_area, n_pos, pos);
}

void geo_mark_overlay(geo_t *g, int n_others, geo_t **others,
		      int *n_overlays, geo_t **overlays) {
    int i, ov_idx;

    ov_idx = 0;
    for(i = 0; i < n_others; i++) {
	if(_is_overlay(g->cur_area, others[i]->cur_area))
	    overlays[ov_idx++] = others[i];
    }
    *n_overlays = ov_idx;
}


#ifdef UNITTEST

#include <CUnit/Basic.h>

void test_geo_from_positions(void) {
    co_aix data[][2] = {
	{33, 25}, {49, 12},
	{14, 28}, {39, 56}};
    geo_t g;
    
    geo_init(&g);
    geo_from_positions(&g, 4, data);
    CU_ASSERT(g.cur_area->x == 14);
    CU_ASSERT(g.cur_area->w == 36);
    CU_ASSERT(g.cur_area->y == 12);
    CU_ASSERT(g.cur_area->h == 45);
}

void test_geo_mark_overlay(void) {
    geo_t _geos[3], *geos[3], *overlays[3];
    geo_t g;
    co_aix pos[2][2];
    int i, n_ov;

    for(i = 0; i < 3; i++) {
	pos[0][0] = i * 50;
	pos[0][1] = i * 50;
	pos[1][0] = i * 50 + 55;
	pos[1][1] = i * 50 + 66;
	geo_init(_geos + i);
	geo_from_positions(_geos + i, 2, pos);
	geos[i] = _geos + i;
    }
    pos[0][0] = 88;
    pos[0][1] = 79;
    pos[1][0] = 88 + 70;
    pos[1][1] = 79 + 70;
    geo_init(&g);
    geo_from_positions(&g, 2, pos);

    /* overlay with geos[1] and geos[2] */
    geo_mark_overlay(&g, 3, geos, &n_ov, overlays);
    CU_ASSERT(n_ov == 2);
    CU_ASSERT(overlays[0] == geos[1]);
    CU_ASSERT(overlays[1] == geos[2]);

    /* right side of geos[1], and up side of geos[2] */
    pos[0][0] = 106;
    pos[0][1] = 51;
    pos[1][0] = 106 + 49;
    pos[1][1] = 51 + 49;
    geo_from_positions(&g, 2, pos);
    geo_mark_overlay(&g, 3, geos, &n_ov, overlays);
    CU_ASSERT(n_ov == 1);
    CU_ASSERT(overlays[0] == geos[2]);
}

CU_pSuite get_geo_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_geo", NULL, NULL);
    CU_ADD_TEST(suite, test_geo_from_positions);
    CU_ADD_TEST(suite, test_geo_mark_overlay);

    return suite;
}

#endif
