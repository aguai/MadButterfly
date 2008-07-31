#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include "mb_types.h"
#include "shapes.h"
#include "tools.h"
#include "redraw_man.h"

#define OK 0
#define ERR -1

#define OFFSET(type, field) ((void *)&((type *)NULL)->field - (void *)NULL)
#define SWAP(a, b, t) do { t c;  c = a; a = b; b = c; } while(0)

int redraw_man_init(redraw_man_t *rdman, cairo_t *cr) {
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
    rdman->n_coords = 1;
    coord_init(rdman->root_coord, NULL);

    rdman->cr = cr;

    return OK;
}

void redraw_man_destroy(redraw_man_t *rdman) {
    elmpool_free(rdman->coord_pool);
    elmpool_free(rdman->geo_pool);
    if(rdman->dirty_coords)
	free(rdman->dirty_coords);
    if(rdman->dirty_geos)
	free(rdman->dirty_geos);
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
    geo_t *visit;
    unsigned int next_order;

    geo = elmpool_elm_alloc(rdman->geo_pool);
    if(geo == NULL)
	return ERR;
    sh_attach_geo(shape, geo);
    STAILQ_INS_TAIL(rdman->all_geos, geo_t, next, geo);
    rdman->n_geos++;

    geo->order = ++rdman->next_geo_order;
    if(geo->order == 0) {
	next_order = 0;
	for(visit = STAILQ_HEAD(rdman->all_geos);
	    visit != NULL;
	    visit = STAILQ_NEXT(geo_t, next, visit))
	    visit->order = ++next_order;
	rdman->next_geo_order = next_order;
    }
    geo->flags |= GEF_DIRTY;

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
    coord_t *coord, *root_coord;
    coord_t *visit;

    coord = elmpool_elm_alloc(rdman->coord_pool);
    if(coord == NULL)
	return NULL;

    coord_init(coord, parent);
    rdman->n_coords++;

    coord->order = ++rdman->next_coord_order;
    if(coord->order == 0) {
	rdman->next_coord_order = 0;
	root_coord = visit = rdman->root_coord;
	/* skip root coord. */
	visit = preorder_coord_subtree(root_coord, visit);
	while(visit) {
	    visit->order = ++rdman->next_coord_order;
	    visit = preorder_coord_subtree(root_coord, visit);
	}
    }

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
    rdman->n_coords--;

    return OK;
}

static int extend_memblk(void **buf, int o_size, int n_size) {
    void *new_buf;

    new_buf = realloc(*buf, n_size);
    if(new_buf == NULL)
	return ERR;

    if(new_buf != *buf) {
	memcpy(new_buf, *buf, o_size);
	free(*buf);
	*buf = new_buf;
    }

    return OK;
}

static int add_dirty_geo(redraw_man_t *rdman, geo_t *geo) {
    int max_dirty_geos;
    int r;

    if(rdman->n_dirty_geos >= rdman->max_dirty_geos) {
	max_dirty_geos = rdman->n_geos;
	r = extend_memblk((void **)&rdman->dirty_geos,
			  sizeof(geo_t *) * rdman->n_dirty_geos,
			  sizeof(geo_t *) * max_dirty_geos);
	if(r != OK)
	    return ERR;
	rdman->max_dirty_geos = max_dirty_geos;
    }

    rdman->dirty_geos[rdman->n_dirty_geos++] = geo;
    return OK;
}

static int add_dirty_area(redraw_man_t *rdman, area_t *area) {
    int max_dirty_areas;
    int r;

    if(rdman->n_dirty_areas >= rdman->max_dirty_areas) {
	/* every geo object and coord object can contribute 2 areas. */
	max_dirty_areas = (rdman->n_geos + rdman->n_coords) * 2;
	r = extend_memblk((void **)&rdman->dirty_areas,
			  sizeof(area_t *) * rdman->n_dirty_areas,
			  sizeof(area_t *) * max_dirty_areas);
	if(r != OK)
	    return ERR;
	rdman->max_dirty_areas = max_dirty_areas;
    }

    rdman->dirty_areas[++rdman->n_dirty_areas] = area;
    return OK;
}

/*! \brief Mark a coord is changed.
 *
 * A changed coord_t object is marked as dirty and put
 * into dirty_coords list.
 */
int rdman_coord_changed(redraw_man_t *rdman, coord_t *coord) {
    int max_dirty_coords;
    int r;

    if(coord->flags & COF_DIRTY)
	return OK;
    
    if(rdman->n_dirty_coords >= rdman->max_dirty_coords) {
	/* Max of dirty_coords is not big enough. */
	max_dirty_coords = rdman->max_dirty_coords + 16;

	r = extend_memblk((void **)&rdman->dirty_coords,
			  sizeof(coord_t *) * rdman->n_dirty_coords,
			  sizeof(coord_t *) * max_dirty_coords);
	rdman->max_dirty_coords = max_dirty_coords;
    }

    rdman->dirty_coords[rdman->n_dirty_coords++] = coord;
    coord->flags |= COF_DIRTY;

    return OK;
}

/*! \brief Mark a shape is changed.
 *
 * The geo_t object of a changed shape is mark as dirty and
 * put into dirty_geos list.
 */
int rdman_shape_changed(redraw_man_t *rdman, shape_t *shape) {
    geo_t *geo;
    int r;

    geo = shape->geo;

    if(geo->flags & GEF_DIRTY)
	return OK;

    r = add_dirty_geo(rdman, geo);
    if(r == ERR)
	return ERR;
    geo->flags |= GEF_DIRTY;

    return OK;
}

/*! \brief Sort a list of element by a unsigned integer.
 *
 * The result is in ascend order.  The unsigned integers is
 * at offset specified by 'off' from start address of elemnts.
 */
static void _insert_sort(void **elms, int num, int off) {
    int i, j;
    unsigned int val;

    for(i = 1; i < num; i++) {
	val = *(unsigned int *)(elms[i] + off);
	for(j = i; j > 0; j--) {
	    if(*(unsigned int *)(elms[j - 1] + off) <= val)
		break;
	    elms[j] = elms[j - 1];
	}
	elms[j] = elms[i];
    }
}

static void update_shape_geo(shape_t *shape) {
    switch(shape->sh_type) {
    case SHT_PATH:
	sh_path_transform(shape);
	break;
    }
}

static void area_to_positions(area_t *area, co_aix (*poses)[2]) {
    poses[0][0] = area->x;
    poses[0][1] = area->y;
    poses[1][0] = area->x + area->w;
    poses[1][1] = area->y + area->h;;
}

static int compute_coord_area(coord_t *coord) {
    shape_t *shape;
    geo_t *geo;
    co_aix (*poses)[2];
    int cnt, pos_cnt;
    int i;

    cnt = 0;
    for(shape = STAILQ_HEAD(coord->members);
	shape != NULL;
	shape = STAILQ_NEXT(shape_t, coord_mem_next, shape)) {
	SWAP(shape->geo->cur_area, shape->geo->last_area, area_t *);
	update_shape_geo(shape);
	cnt++;
    }

    poses = (co_aix (*)[2])malloc(sizeof(co_aix [2]) * 2 * cnt);
    if(poses == NULL)
	return ERR;

    pos_cnt = 0;
    for(shape = STAILQ_HEAD(coord->members);
	shape != NULL;
	shape = STAILQ_NEXT(shape_t, coord_mem_next, shape)) {
	geo = shape->geo;
	
	area_to_positions(&geo->areas[0], poses + pos_cnt);
	pos_cnt += 2;
	area_to_positions(&geo->areas[1], poses + pos_cnt);
	pos_cnt += 2;
    }

    for(i = 0; i < pos_cnt; i++)
	coord_trans_pos(coord, &poses[i][0], &poses[i][1]);

    area_init(coord->cur_area, cnt, poses);
    free(poses);

    return OK;
}

static void draw_shape(redraw_man_t *rdman, shape_t *shape) {
    switch(shape->sh_type) {
    case SHT_PATH:
	sh_path_draw(shape, rdman->cr);
	break;
    }
}

static void make_clip(redraw_man_t *rdman, int n_dirty_areas,
		      area_t **dirty_areas) {
    int i;
    area_t *area;
    cairo_t *cr;

    cr = rdman->cr;

    cairo_reset_clip(cr);
    for(i = 0; i < n_dirty_areas; i++) {
	area = dirty_areas[i];
	cairo_rectangle(cr, area->x, area->y, area->w, area->h);
    }
    cairo_clip(cr);
}

static void draw_shapes_in_areas(redraw_man_t *rdman,
				 int n_areas,
				 area_t **areas) {
    geo_t *visit_geo;
    int i;

    for(visit_geo = STAILQ_HEAD(rdman->all_geos);
	visit_geo != NULL;
	visit_geo = STAILQ_NEXT(geo_t, next, visit_geo)) {
	if(visit_geo->flags & GEF_DIRTY) {
	    visit_geo->flags &= ~GEF_DIRTY;
	    update_shape_geo(visit_geo->shape);
	}
	for(i = 0; i < n_areas; i++) {
	    if(is_overlay(visit_geo->cur_area, areas[i])) {
		draw_shape(rdman, visit_geo->shape);
		break;
	    }
	}
    }
}

/*! \brief Re-draw all changed shapes or shapes affected by changed coords.
 *
 * A coord object has a geo to keep track the range that it's members will
 * draw on.  Geo of a coord should be recomputed when the coord is changed.
 * Geo of a coord used to accelerate finding overlay shape objects of
 * a specified geo.  A coord object also must be recomputed when one of
 * it's members is changed.
 *
 * New and old geo values of a coord object that is recomputed for
 * changing of it-self must be used to find overlay shape objects.
 * New and old geo values of a shape should also be used to find
 * overlay shape objects, too.  If a shape's coord is changed, shape's
 * geo object is not used to find overlay shape objects any more.
 *
 * steps:
 * - update chagned coord objects
 * - recompute area for changed coord objects
 *   - recompute geo for members shape objects
 *   - clear dirty of geo for members to prevent from
 *     recomputing for change of shape objects.
 *   - add old and new area value to list of dirty areas.
 * - recompute geo for changed shape objects
 *   - only if a shape object is dirty.
 *   - put new and old value of area of geo to list of dirty areas.
 * - Scan all shapes and redraw shapes overlaid with dirty areas.
 *
 * dirty flag of coord objects is cleared after update.
 * dirty flag of geo objects is also cleared after recomputing.
 * Clean dirty flag can prevent redundant computing for geo and
 * corod objects.
 *
 */
int rdman_redraw_changed(redraw_man_t *rdman) {
    int i, r;
    int n_dirty_coords;
    coord_t **dirty_coords;
    coord_t *visit_coord;
    geo_t *visit_geo, **dirty_geos;
    int n_dirty_geos;
    int n_dirty_areas;
    area_t **dirty_areas;

    if(rdman->n_dirty_coords > 0) {
	_insert_sort((void **)rdman->dirty_coords,
		     rdman->n_dirty_coords,
		     OFFSET(coord_t, order));
	n_dirty_coords = rdman->n_dirty_coords;
	dirty_coords = rdman->dirty_coords;
	for(i = 0; i < n_dirty_coords; i++) {
	    if(!(dirty_coords[i]->flags & COF_DIRTY))
		continue;

	    update_aggr_matrix(dirty_coords[i]);
	    for(visit_coord = dirty_coords[i];
		visit_coord != NULL;
		visit_coord = preorder_coord_subtree(dirty_coords[i],
						     visit_coord)) {
		/* Dirty member, here, and members of this coord
		 * will not be visited anymore. */
		visit_coord->flags &= ~COF_DIRTY;

		SWAP(visit_coord->cur_area, visit_coord->last_area, area_t *);
		r = compute_coord_area(visit_coord);
		if(r == ERR)
		    return ERR;
		add_dirty_area(rdman, visit_coord->cur_area);
		add_dirty_area(rdman, visit_coord->last_area);
	    }
	}
	rdman->n_dirty_coords = 0;
    }

    n_dirty_geos = rdman->n_dirty_geos;
    if(n_dirty_geos > 0) {
	dirty_geos = rdman->dirty_geos;
	for(i = 0; i < n_dirty_geos; i++) {
	    visit_geo = dirty_geos[i];
	    if(!(visit_geo->flags & GEF_DIRTY))
		continue;

	    visit_geo->flags &= ~GEF_DIRTY;
	    SWAP(visit_geo->cur_area, visit_geo->last_area, area_t *);
	    update_shape_geo(visit_geo->shape);
	    add_dirty_area(rdman, visit_geo->cur_area);
	    add_dirty_area(rdman, visit_geo->last_area);
	}
	rdman->n_dirty_geos = 0;
    }
	
    n_dirty_areas = rdman->n_dirty_areas;
    dirty_areas = rdman->dirty_areas;
    if(n_dirty_areas > 0) {
	make_clip(rdman, n_dirty_areas, dirty_areas);
	draw_shapes_in_areas(rdman, n_dirty_areas, dirty_areas);
	rdman->n_dirty_areas = 0;
    }

    return OK;
}

int rdman_redraw_all(redraw_man_t *rdman) {
    geo_t *geo;

    /* TODO: update dirty coord and it's members. */
    for(geo = STAILQ_HEAD(rdman->all_geos);
	geo != NULL;
	geo = STAILQ_NEXT(geo_t, next, geo)) {
	if(geo->flags & GEF_DIRTY) {
	    geo->flags &= ~GEF_DIRTY;
	    update_shape_geo(geo->shape);
	}
	draw_shape(rdman, geo->shape);
    }

    return OK;
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

/*
 * functions:
 * - redraw all
 * - redraw changed
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
    co_aix pos[2][2];
    int n;
    int i;

    redraw_man_init(&rdman, NULL);
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

    pos[0][0] = 100;
    pos[0][1] = 120;
    pos[1][0] = 100 + 140;
    pos[1][1] = 120 + 40;
    geo_init(&geo, 2, pos);

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
