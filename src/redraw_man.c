#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mb_types.h"
#include "shapes.h"
#include "tools.h"
#include "redraw_man.h"

#define OK 0
#define ERR -1

int redraw_man_init(redraw_man_t *rdman) {
    extern void redraw_man_destroy(redraw_man_t *rdman);

    memset(rdman, 0, sizeof(redraw_man_t));

    rdman->geo_pool = elmpool_new(sizeof(geo_t), 128);
    if(rdman->geo_pool == NULL)
	return ERR;

    rdman->coord_pool = elmpool_new(sizeof(coord_t), 16);
    if(rdman->coord_pool == NULL) {
	elmpool_free(rdman->geo_pool);
	return ERR;
    }

    rdman->root_coord = elmpool_elm_alloc(rdman->coord_pool);
    if(rdman->root_coord == NULL)
	redraw_man_destroy(rdman);

    coord_init(rdman->root_coord, NULL);

    return OK;
}

void redraw_man_destroy(redraw_man_t *rdman) {
    elmpool_free(rdman->coord_pool);
    elmpool_free(rdman->geo_pool);
}


#define ASSERT(x)
/*
 * Change transformation matrix
 * - update aggregated transformation matrix
 *   - of coord_t object been changed.
 *   - of children coord_t objects.
 * - redraw members of coord_t objects.
 * - redraw shape objects they are overlaid with members.
 *   - find out overlaid shape objects.
 *   - geo_t of a coord_t object
 *     - can make finding more efficiency.
 *     - fill overlay geo_t objects of members.
 *
 * Change a shape object
 * - redraw changed object.
 * - redraw shape objects they are overlaid with changed object.
 *   - find out overlaid shape objects.
 *
 * That coord and geo of shape objects are setted by user code
 * give user code a chance to collect coord and geo objects together
 * and gain interest of higher cache hit rate.
 */

/*! \brief Find out all affected shape objects.
 *
 * Find out all shape objects that are overalid with geo_t of
 * a geometry changed object.
 *
 * Linear scan geo_t objects of all shape objects in all_shapes
 * list of a redraw_man_t object.
 */
int rdman_find_overlaid_shapes(redraw_man_t *rdman, geo_t *geo,
			 geo_t ***overlays) {
    int n_geos;
    geo_t **geos;
    geo_t *geo_cur;
    int n_overlays;
    geo_t **_overlays;
    int i;

    n_geos = rdman->n_geos;

    geos = (geo_t **)malloc(sizeof(geo_t *) * n_geos);
    if(geos == NULL)
	return -1;

    _overlays = (geo_t **)malloc(sizeof(geo_t *) * n_geos);
    if(geos == NULL) {
	free(geos);
	return -1;
    }

    geo_cur = STAILQ_HEAD(rdman->all_geos);
    for(i = 0; i < n_geos; i++) {
	geos[i] = geo_cur;
	geo_cur = STAILQ_NEXT(geo_t, next, geo_cur);
    }
    geo_mark_overlay(geo, n_geos, geos, &n_overlays, _overlays);

    free(geos);
    *overlays = _overlays;

    return n_overlays;
}

int rdman_add_shape(redraw_man_t *rdman, shape_t *shape, coord_t *coord) {
    geo_t *geo;

    geo = elmpool_elm_alloc(rdman->geo_pool);
    if(geo == NULL)
	return ERR;
    sh_attach_geo(shape, geo);
    STAILQ_INS_TAIL(rdman->all_geos, geo_t, next, geo);
    rdman->n_geos++;
    sh_attach_coord(shape, coord);

    return OK;
}

int rdman_remove_shape(redraw_man_t *rdman, shape_t *shape) {
    STAILQ_REMOVE(rdman->all_geos, geo_t, next, shape->geo);
    elmpool_elm_free(rdman->geo_pool, shape->geo);
    sh_detach_geo(shape);
    rdman->n_geos--;
    sh_detach_coord(shape);
    return OK;
}

coord_t *rdman_coord_new(redraw_man_t *rdman, coord_t *parent) {
    coord_t *coord;

    coord = elmpool_elm_alloc(rdman->coord_pool);
    if(coord == NULL)
	return NULL;

    coord_init(coord, parent);

    return coord;
}

/*! \brief Free a coord of a redraw_man_t object.
 *
 * \param coord is a coord_t without children and members.
 * \return 0 for successful, -1 for error.
 */
int rdman_coord_free(redraw_man_t *rdman, coord_t *coord) {
    coord_t *parent;

    parent = coord->parent;
    if(parent == NULL)
	return ERR;

    if(STAILQ_HEAD(coord->members) != NULL)
	return ERR;

    if(STAILQ_HEAD(coord->children) != NULL)
	return ERR;

    STAILQ_REMOVE(parent->children, coord_t, sibling, coord);
    elmpool_elm_free(rdman->coord_pool, coord);

    return OK;
}

void rdman_coord_changed(redraw_man_t *rdman, coord_t *coord) {
}

/*
 * When redraw an area, the affected elements may also extend to
 * outside of the area.  Since the order of drawing will change
 * the result, it will infect more and more elements to keep
 * drawing order althrough they are overlaid directly with
 * specified area.
 *
 * To fix the problem, we don't extend the set of redrawing to
 * elements they are not overliad directly.  The redrawing is
 * performed on a temporary surface, clipped to fit the area, and
 * update only specified area on the destinate surface.
 */

/*
 * To accelerate speed of transformation, when a matrix changed,
 * transformation should be aggregated and computed in a loop.
 * It can get intereset of higher hit rate of cache.
 * - shapes prvoide list of positions needed to be transformed.
 * - redraw_man transforms positions from shapes.
 * - shapes drawing with result of transforms.
 * - shapes should be called to give them a chance to update geometries.
 */

#ifdef UNITTEST

#include <CUnit/Basic.h>

struct _sh_dummy {
    shape_t shape;
    co_aix x, y;
    co_aix w, h;
};
typedef struct _sh_dummy sh_dummy_t;

shape_t *sh_dummy_new(co_aix x, co_aix y, co_aix w, co_aix h) {
    sh_dummy_t *dummy;

    dummy = (sh_dummy_t *)malloc(sizeof(sh_dummy_t));
    if(dummy == NULL)
	return NULL;

    dummy->x = x;
    dummy->y = y;
    dummy->w = w;
    dummy->h = h;

    return (shape_t *)dummy;
}

void sh_dummy_free(shape_t *sh) {
    free(sh);
}

void sh_dummy_transform(shape_t *shape) {
    sh_dummy_t *dummy = (sh_dummy_t *)shape;
    co_aix poses[2][2];
    co_aix x1, y1, x2, y2;
    
    if(shape->geo && shape->coord) {
	x1 = dummy->x;
	y1 = dummy->y;
	x2 = x1 + dummy->w;
	y2 = y1 + dummy->h;

	coord_trans_pos(shape->coord, &x1, &y1);
	coord_trans_pos(shape->coord, &x2, &y2);
	poses[0][0] = x1;
	poses[0][1] = y1;
	poses[1][0] = x2;
	poses[1][1] = y2;
    
	geo_init(shape->geo, 2, poses);
    }
}

void test_rdman_find_overlaid_shapes(void) {
    redraw_man_t rdman;
    geo_t geo;
    coord_t *coords[3];
    shape_t *shapes[5];
    geo_t **overlays;
    int n;
    int i;

    redraw_man_init(&rdman);
    coords[0] = rdman.root_coord;
    for(i = 1; i < 3; i++) {
	coords[i] = rdman_coord_new(&rdman, rdman.root_coord);
    }
    for(i = 0; i < 5; i++) {
	shapes[i] = sh_dummy_new(10 + i * 30, 10 + i * 20, 25, 15);
	CU_ASSERT(shapes[i] != NULL);
    }
    for(i = 0; i < 3; i++)
	rdman_add_shape(&rdman, shapes[i], coords[1]);
    for(i = 3; i < 5; i++)
	rdman_add_shape(&rdman, shapes[i], coords[2]);

    coords[1]->matrix[0] = 2;
    coords[0]->matrix[4] = 2;

    update_aggr_matrix(coords[0]);
    for(i = 0; i < 5; i++)
	sh_dummy_transform(shapes[i]);

    geo.x = 100;
    geo.y = 120;
    geo.w = 140;
    geo.h = 40;

    n = rdman_find_overlaid_shapes(&rdman, &geo, &overlays);
    CU_ASSERT(n == 2);
    CU_ASSERT(overlays != NULL);
    CU_ASSERT(overlays[0] == shapes[2]->geo);
    CU_ASSERT(overlays[1] == shapes[3]->geo);

    free(overlays);
    for(i = 0; i < 5; i++)
	sh_dummy_free(shapes[i]);

    redraw_man_destroy(&rdman);
}

CU_pSuite get_redraw_man_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_redraw_man", NULL, NULL);
    CU_ADD_TEST(suite, test_rdman_find_overlaid_shapes);

    return suite;
}

#endif /* UNITTEST */
