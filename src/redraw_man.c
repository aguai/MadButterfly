#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include "mb_types.h"
#include "shapes.h"
#include "tools.h"
#include "redraw_man.h"
#include "observer.h"


/* NOTE: bounding box should also consider width of stroke.
 */

#define OK 0
#define ERR -1

#define ARRAY_EXT_SZ 64

#define SWAP(a, b, t) do { t c;  c = a; a = b; b = c; } while(0)

#ifdef UNITTEST
typedef struct _sh_dummy sh_dummy_t;

extern void sh_dummy_transform(shape_t *shape);
extern void sh_dummy_fill(shape_t *, cairo_t *);
#endif /* UNITTEST */

static subject_t *ob_subject_alloc(ob_factory_t *factory);
static void ob_subject_free(ob_factory_t *factory, subject_t *subject);
static observer_t *ob_observer_alloc(ob_factory_t *factory);
static void ob_observer_free(ob_factory_t *factory, observer_t *observer);
static subject_t *ob_get_parent_subject(ob_factory_t *factory,
					subject_t *cur_subject);
/* Functions for children. */
#define FORCHILDREN(coord, child)				\
    for((child) = STAILQ_HEAD((coord)->children);		\
	(child) != NULL;					\
	(child) = STAILQ_NEXT(coord_t, sibling, (child)))
#define NEXT_CHILD(child) STAILQ_NEXT(coord_t, sibling, child)
#define ADD_CHILD(parent, child)					\
    STAILQ_INS_TAIL((parent)->children, coord_t, sibling, (child))
#define RM_CHILD(parent, child)						\
    STAILQ_REMOVE((parent)->children, coord_t, sibling, (child))
#define FIRST_CHILD(parent) STAILQ_HEAD((parent)->children)

/* Functions for members. */
#define FORMEMBERS(coord, member)				\
    for((member) = STAILQ_HEAD((coord)->members);		\
	(member) != NULL;					\
	(member) = STAILQ_NEXT(geo_t, coord_next, (member)))
#define NEXT_MEMBER(member) STAILQ_NEXT(geo_t, coord_next, (member))
#define ADD_MEMBER(coord, member)					\
    STAILQ_INS_TAIL((coord)->members, geo_t, coord_next, (member))
#define RM_MEMBER(coord, member)					\
    STAILQ_REMOVE((coord)->members, geo_t, coord_next, (member))
#define FIRST_MEMBER(coord) STAILQ_HEAD((coord)->members)

/*! \brief Sort a list of element by a unsigned integer.
 *
 * The result is in ascend order.  The unsigned integers is
 * at offset specified by 'off' from start address of elemnts.
 */
static void _insert_sort(void **elms, int num, int off) {
    int i, j;
    unsigned int val;
    void *elm_i;

    for(i = 1; i < num; i++) {
	elm_i = elms[i];
	val = *(unsigned int *)(elm_i + off);
	for(j = i; j > 0; j--) {
	    if(*(unsigned int *)(elms[j - 1] + off) <= val)
		break;
	    elms[j] = elms[j - 1];
	}
	elms[j] = elm_i;
    }
}

static int extend_memblk(void **buf, int o_size, int n_size) {
    void *new_buf;

    new_buf = realloc(*buf, n_size);
    if(new_buf == NULL)
	return ERR;

    *buf = new_buf;

    return OK;
}

static int add_dirty_geo(redraw_man_t *rdman, geo_t *geo) {
    int max_dirty_geos;
    int r;

    if(rdman->n_dirty_geos >= rdman->max_dirty_geos) {
	max_dirty_geos = rdman->max_dirty_geos + ARRAY_EXT_SZ;
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
	/* every geo object and coord object can contribute 2 areas.
	 * rdman_draw_area() may also contribute 1 area.
	 */
	max_dirty_areas = rdman->max_dirty_areas + ARRAY_EXT_SZ;
	r = extend_memblk((void **)&rdman->dirty_areas,
			  sizeof(area_t *) * rdman->n_dirty_areas,
			  sizeof(area_t *) * max_dirty_areas);
	if(r != OK)
	    return ERR;
	rdman->max_dirty_areas = max_dirty_areas;
    }

    rdman->dirty_areas[rdman->n_dirty_areas++] = area;
    return OK;
}

static void area_to_positions(area_t *area, co_aix (*poses)[2]) {
    poses[0][0] = area->x;
    poses[0][1] = area->y;
    poses[1][0] = area->x + area->w;
    poses[1][1] = area->y + area->h;;
}

static cairo_t *new_canvas(redraw_man_t *rdman) {
#ifndef UNITTEST
    cairo_t *cr;
    cairo_surface_t *surface, *cr_surface;
    int w, h;

    cr_surface = cairo_get_target(rdman->cr);
    w = cairo_image_surface_get_width(cr_surface);
    h = cairo_image_surface_get_height(cr_surface);
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					 w, h);
    cr = cairo_create(surface);

    return cr;
#else
    return NULL;
#endif
}

static void free_canvas(cairo_t *canvas) {
#ifndef UNITTEST
    cairo_destroy(canvas);
#endif
}

static int geo_off_in_coord(geo_t *geo, coord_t *coord) {
    int off = 0;
    geo_t *vgeo;

    FORMEMBERS(coord, vgeo) {
	if(vgeo == geo)
	    return off;
	off++;
    }
    return -1;
}

static void geo_attach_coord(geo_t *geo, coord_t *coord) {
    ADD_MEMBER(coord, geo);
    coord->num_members++;
}

static void geo_detach_coord(geo_t *geo, coord_t *coord) {
    int off;
    coord_t *child;

    off = geo_off_in_coord(geo, coord);
    if(off < 0)
	return;
    FORCHILDREN(coord, child) {
	if(child->before_pmem >= off)
	    child->before_pmem--;
    }

    RM_MEMBER(coord, geo);
    coord->num_members--;
}

int redraw_man_init(redraw_man_t *rdman, cairo_t *cr, cairo_t *backend) {
    extern void redraw_man_destroy(redraw_man_t *rdman);
    extern int paint_color_size;

    memset(rdman, 0, sizeof(redraw_man_t));

    rdman->geo_pool = elmpool_new(sizeof(geo_t), 128);
    if(rdman->geo_pool == NULL)
	return ERR;

    rdman->coord_pool = elmpool_new(sizeof(coord_t), 16);
    if(rdman->coord_pool == NULL) {
	elmpool_free(rdman->geo_pool);
	return ERR;
    }

    rdman->shnode_pool = elmpool_new(sizeof(shnode_t), 16);
    if(rdman->shnode_pool == NULL) {
	elmpool_free(rdman->geo_pool);
	elmpool_free(rdman->coord_pool);
	return ERR;
    }

    rdman->observer_pool = elmpool_new(sizeof(observer_t), 32);
    if(rdman->observer_pool == NULL) {
	elmpool_free(rdman->geo_pool);
	elmpool_free(rdman->coord_pool);
	elmpool_free(rdman->shnode_pool);
	return ERR;
    }

    rdman->subject_pool = elmpool_new(sizeof(subject_t), 32);
    if(rdman->subject_pool == NULL) {
	elmpool_free(rdman->geo_pool);
	elmpool_free(rdman->coord_pool);
	elmpool_free(rdman->shnode_pool);
	elmpool_free(rdman->observer_pool);
	return ERR;
    }

    rdman->paint_color_pool = elmpool_new(paint_color_size, 64);
    if(rdman->subject_pool == NULL) {
	elmpool_free(rdman->geo_pool);
	elmpool_free(rdman->coord_pool);
	elmpool_free(rdman->shnode_pool);
	elmpool_free(rdman->observer_pool);
	elmpool_free(rdman->subject_pool);
	return ERR;
    }

    rdman->ob_factory.subject_alloc = ob_subject_alloc;
    rdman->ob_factory.subject_free = ob_subject_free;
    rdman->ob_factory.observer_alloc = ob_observer_alloc;
    rdman->ob_factory.observer_free = ob_observer_free;
    rdman->ob_factory.get_parent_subject = ob_get_parent_subject;

    rdman->root_coord = elmpool_elm_alloc(rdman->coord_pool);
    if(rdman->root_coord == NULL)
	redraw_man_destroy(rdman);
    rdman->n_coords = 1;
    coord_init(rdman->root_coord, NULL);
    rdman->root_coord->mouse_event = subject_new(&rdman->ob_factory,
						 rdman->root_coord,
						 OBJT_COORD);
    rdman->root_coord->flags |= COF_OWN_CANVAS;
    rdman->root_coord->canvas = cr;
    rdman->root_coord->opacity = 1;

    rdman->cr = cr;
    rdman->backend = backend;

    return OK;
}

void redraw_man_destroy(redraw_man_t *rdman) {
    coord_t *coord, *saved_coord;

    coord = postorder_coord_subtree(rdman->root_coord, NULL);
    while(coord) {
	saved_coord = coord;
	coord = postorder_coord_subtree(rdman->root_coord, coord);
	rdman_coord_free(rdman, saved_coord);
    }

    elmpool_free(rdman->coord_pool);
    elmpool_free(rdman->geo_pool);
    elmpool_free(rdman->shnode_pool);
    elmpool_free(rdman->observer_pool);
    elmpool_free(rdman->subject_pool);
    elmpool_free(rdman->paint_color_pool);
    if(rdman->dirty_coords)
	free(rdman->dirty_coords);
    if(rdman->dirty_geos)
	free(rdman->dirty_geos);
    if(rdman->gen_geos)
	free(rdman->gen_geos);
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

int rdman_add_shape(redraw_man_t *rdman, shape_t *shape, coord_t *coord) {
    geo_t *geo;
    int r;

    geo = elmpool_elm_alloc(rdman->geo_pool);
    if(geo == NULL)
	return ERR;
    
    geo_init(geo);
    geo->mouse_event = subject_new(&rdman->ob_factory, geo, OBJT_GEO);

    geo_attach_coord(geo, coord);

    /* New one should be dirty to recompute it when drawing. */
    geo->flags |= GEF_DIRTY;
    r = add_dirty_geo(rdman, geo);
    if(r != OK)
	return ERR;

    sh_attach_coord(shape, coord);
    sh_attach_geo(shape, geo);

    return OK;
}

/*! \brief Remove a shape object from redraw manager.
 *
 * \todo redraw shape objects that overlaid with removed one.
 */
int rdman_remove_shape(redraw_man_t *rdman, shape_t *shape) {
    geo_t *geo;
    coord_t *coord;

    geo = shape->geo;
    coord = shape->coord;
    geo_detach_coord(geo, coord);
    subject_free(&rdman->ob_factory, geo->mouse_event);
    sh_detach_geo(shape);
    elmpool_elm_free(rdman->geo_pool, shape->geo);
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
    coord->mouse_event = subject_new(&rdman->ob_factory,
				     coord,
				     OBJT_COORD);
    /*! \note default opacity == 1 */
    coord->opacity = 1;
    if(parent)
	coord->canvas = parent->canvas;
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

    coord->before_pmem = parent->num_members;

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

    if(FIRST_MEMBER(coord) != NULL)
	return ERR;

    if(FIRST_CHILD(coord) != NULL)
	return ERR;

    /* Free canvas (\ref redraw) */
    if(coord->flags & COF_OWN_CANVAS)
	free_canvas(coord->canvas);

    RM_CHILD(parent, coord);
    subject_free(&rdman->ob_factory, coord->mouse_event);
    elmpool_elm_free(rdman->coord_pool, coord);
    rdman->n_coords--;

    return OK;
}

static void make_sure_dirty_coords(redraw_man_t *rdman) {
    int max_dirty_coords;
    int r;
    
    if(rdman->n_dirty_coords >= rdman->max_dirty_coords) {
	/* Max of dirty_coords is not big enough. */
	max_dirty_coords = rdman->max_dirty_coords + 16;
	
	r = extend_memblk((void **)&rdman->dirty_coords,
			  sizeof(coord_t *) * rdman->n_dirty_coords,
			  sizeof(coord_t *) * max_dirty_coords);
	rdman->max_dirty_coords = max_dirty_coords;
    }
}

/*! \brief Mark a coord is changed.
 *
 * A changed coord_t object is marked as dirty and put
 * into dirty_coords list.
 */
int rdman_coord_changed(redraw_man_t *rdman, coord_t *coord) {
    coord_t *child;

    if(coord->flags & COF_DIRTY)
	return OK;
    
    /* Make the coord and child coords dirty. */
    for(child = coord;
	child != NULL;
	child = preorder_coord_subtree(coord, child)) {
	if(child->flags & COF_DIRTY)
	    continue;
	make_sure_dirty_coords(rdman);
 
	rdman->dirty_coords[rdman->n_dirty_coords++] = child;
	child->flags |= COF_DIRTY;
    }

    return OK;
}

static int _rdman_shape_changed(redraw_man_t *rdman, shape_t *shape) {
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

/*! \brief Mark a shape is changed.
 *
 * The geo_t object of a changed shape is mark as dirty and
 * put into dirty_geos list.
 */
int rdman_shape_changed(redraw_man_t *rdman, shape_t *shape) {
    return _rdman_shape_changed(rdman, shape);
}

int rdman_paint_changed(redraw_man_t *rdman, paint_t *paint) {
    shnode_t *node;
    int r;

    for(node = STAILQ_HEAD(paint->members);
	node != NULL;
	node = STAILQ_NEXT(shnode_t, next, node)) {
	r = _rdman_shape_changed(rdman, node->shape);
	if(r != OK)
	    return ERR;
    }
    return OK;
}

/* Clean dirties */

static int is_coord_subtree_hidden(coord_t *coord) {
    while(coord) {
	if(coord->flags & COF_HIDDEN)
	    return 1;
	coord = coord->parent;
    }
    return 0;
}

static void clean_shape(shape_t *shape) {
    switch(shape->sh_type) {
    case SHT_PATH:
	sh_path_transform(shape);
	break;
    case SHT_TEXT:
	sh_text_transform(shape);
	break;
    case SHT_RECT:
	sh_rect_transform(shape);
	break;
#ifdef UNITTEST
    default:
	sh_dummy_transform(shape);
	break;
#endif /* UNITTEST */
    }
    shape->geo->flags &= ~GEF_DIRTY;

    if(is_coord_subtree_hidden(shape->coord))
	shape->geo->flags |= GEF_HIDDEN;
    else
	shape->geo->flags &= ~GEF_HIDDEN;
}

/*! \brief Setup canvas for the coord.
 *
 * Own a canvas or inherit it from parent.
 * \sa
 * - \ref redraw
 */
static void setup_canvas(redraw_man_t *rdman, coord_t *coord) {
    if(coord->parent == NULL)
	return;

    if(coord->opacity != 1) {
	if(!(coord->flags & COF_OWN_CANVAS)) {
	    coord->canvas = new_canvas(rdman);
	    coord->flags |= COF_OWN_CANVAS;
	}
    } else {
	if(coord->flags & COF_OWN_CANVAS) {
	    free_canvas(coord->canvas);
	    coord->flags &= ~COF_OWN_CANVAS;
	}
	coord->canvas = coord->parent->canvas;
    }
}

/*! \todo Use a static variable to hold positions array for clean_coord()? */
static int clean_coord(redraw_man_t *rdman, coord_t *coord) {
    geo_t *geo;
    co_aix (*poses)[2];
    int cnt, pos_cnt;

    setup_canvas(rdman, coord);

    compute_aggr_of_coord(coord);

    /* Clean member shapes. */
    cnt = 0;
    FORMEMBERS(coord, geo) {
	SWAP(geo->cur_area, geo->last_area, area_t *);
	clean_shape(geo->shape);
	cnt++;
    }

    /* Compute area of the coord. */
    poses = (co_aix (*)[2])malloc(sizeof(co_aix [2]) * 2 * cnt);
    if(poses == NULL)
	return ERR;

    pos_cnt = 0;
    FORMEMBERS(coord, geo) {
	area_to_positions(geo->cur_area, poses + pos_cnt);
	pos_cnt += 2;
    }

    SWAP(coord->cur_area, coord->last_area, area_t *);
    area_init(coord->cur_area, pos_cnt, poses);
    free(poses);
    
    coord->flags &= ~COF_DIRTY;

    return OK;
}

/*! \brief Clean coord_t objects.
 */
static int clean_rdman_coords(redraw_man_t *rdman) {
    coord_t *coord;
    coord_t **dirty_coords;
    int n_dirty_coords;
    int i, r;

    n_dirty_coords = rdman->n_dirty_coords;
    if(n_dirty_coords > 0) {
	dirty_coords = rdman->dirty_coords;
	_insert_sort((void **)dirty_coords, n_dirty_coords,
		     OFFSET(coord_t, order));
	for(i = 0; i < n_dirty_coords; i++) {
	    coord = dirty_coords[i];
	    if(!(coord->flags & COF_DIRTY))
		continue;
	    r = clean_coord(rdman, coord);
	    if(r != OK)
		return ERR;
	    /* These two steps can be avoided for drawing all. */
	    add_dirty_area(rdman, &coord->areas[0]);
	    add_dirty_area(rdman, &coord->areas[1]);
	}
	rdman->n_dirty_coords = 0;
    }
    return OK;
}

static int clean_rdman_geos(redraw_man_t *rdman) {
    int i;
    int n_dirty_geos;
    geo_t **dirty_geos;
    geo_t *visit_geo;

    n_dirty_geos = rdman->n_dirty_geos;
    if(n_dirty_geos > 0) {
	dirty_geos = rdman->dirty_geos;
	for(i = 0; i < n_dirty_geos; i++) {
	    visit_geo = dirty_geos[i];
	    if(!(visit_geo->flags & GEF_DIRTY))
		continue;

	    SWAP(visit_geo->cur_area, visit_geo->last_area, area_t *);
	    clean_shape(visit_geo->shape);
	    add_dirty_area(rdman, visit_geo->cur_area);
	    add_dirty_area(rdman, visit_geo->last_area);
	}
	rdman->n_dirty_geos = 0;
    }    

    return OK;
}

static int clean_rdman_dirties(redraw_man_t *rdman) {
    int r;

    r = clean_rdman_coords(rdman);
    if(r != OK)
	return ERR;

    r = clean_rdman_geos(rdman);
    if(r != OK)
	return ERR;

    return OK;
}


/* Drawing and Redrawing
 * ============================================================
 */

#ifndef UNITTEST
static void set_shape_stroke_param(shape_t *shape, cairo_t *cr) {
    cairo_set_line_width(cr, shape->stroke_width);
}

static void fill_path_preserve(redraw_man_t *rdman) {
    cairo_fill_preserve(rdman->cr);
}

static void fill_path(redraw_man_t *rdman) {
    cairo_fill(rdman->cr);
}

static void stroke_path(redraw_man_t *rdman) {
    cairo_stroke(rdman->cr);
}
#else
static void set_shape_stroke_param(shape_t *shape, cairo_t *cr) {
}

static void fill_path_preserve(redraw_man_t *rdman) {
}

static void fill_path(redraw_man_t *rdman) {
}

static void stroke_path(redraw_man_t *rdman) {
}
#endif

static void draw_shape(redraw_man_t *rdman, cairo_t *cr, shape_t *shape) {
    paint_t *fill, *stroke;

    if(shape->fill || shape->stroke) {
	switch(shape->sh_type) {
	case SHT_PATH:
	    sh_path_draw(shape, cr);
	    break;
	case SHT_TEXT:
	    sh_text_draw(shape, cr);
	    break;
	case SHT_RECT:
	    sh_rect_draw(shape, cr);
	    break;
#ifdef UNITTEST
	default:
	    sh_dummy_fill(shape, cr);
	    break;
#endif /* UNITTEST */
	}

	fill = shape->fill;
	if(shape->fill) {
	    fill->prepare(fill, cr);
	    if(shape->stroke)
		fill_path_preserve(rdman);
	    else
		fill_path(rdman);
	}

	stroke = shape->stroke;
	if(stroke) {
	    stroke->prepare(stroke, cr);
	    set_shape_stroke_param(shape, cr);
	    stroke_path(rdman);
	}
    }
}

#ifndef UNITTEST
static void clean_canvas(cairo_t *cr) {
    /*! \todo clean to background color. */
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
}

static void clean_canvas_black(cairo_t *cr) {
    /*! \todo clean to background color. */
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
}

static void make_clip(cairo_t *cr, int n_dirty_areas,
		      area_t **dirty_areas) {
    int i;
    area_t *area;

    for(i = 0; i < n_dirty_areas; i++) {
	area = dirty_areas[i];
	cairo_rectangle(cr, area->x, area->y, area->w, area->h);
    }
    cairo_clip(cr);
}

static void reset_clip(redraw_man_t *rdman) {
    cairo_reset_clip(rdman->backend);
}

static void copy_cr_2_backend(redraw_man_t *rdman, int n_dirty_areas,
			      area_t **dirty_areas) {
    if(n_dirty_areas)
	make_clip(rdman->backend, n_dirty_areas, dirty_areas);
    
    cairo_paint(rdman->backend);
}
#else /* UNITTEST */
static void clean_canvas(cairo_t *cr) {
}

static void clean_canvas_black(cairo_t *cr) {
}

static void reset_clip(redraw_man_t *rdman) {
}

static void copy_cr_2_backend(redraw_man_t *rdman, int n_dirty_areas,
			      area_t **dirty_areas) {
}
#endif /* UNITTEST */

static int is_geo_in_areas(geo_t *geo,
			     int n_areas,
			     area_t **areas) {
    int i;

    for(i = 0; i < n_areas; i++) {
	if(is_overlay(geo->cur_area, areas[i]))
	    return 1;
    }
    return 0;
}

static void update_canvas_2_parent(redraw_man_t *rdman, coord_t *coord) {
    cairo_t *pcanvas, *canvas;
    cairo_surface_t *surface;

    if(coord == rdman->root_coord)
	return;

    canvas = coord->canvas;
    pcanvas = coord->parent->canvas;
    surface = cairo_get_target(canvas);
    cairo_set_source_surface(pcanvas, surface, 0, 0);
    cairo_paint_with_alpha(pcanvas, coord->opacity);
}

static int draw_coord_shapes_in_areas(redraw_man_t *rdman,
				       coord_t *coord,
				       int n_areas,
				       area_t **areas) {
    int dirty = 0;
    int r;
    geo_t *member;
    coord_t *child;
    cairo_t *canvas;
    int mem_idx;

    canvas = coord->canvas;
    member = FIRST_MEMBER(coord);
    mem_idx = 0;
    child = FIRST_CHILD(coord);
    while(child != NULL || member != NULL) {
	if(child && child->before_pmem == mem_idx) {
	    r = draw_coord_shapes_in_areas(rdman, child, n_areas, areas);
	    dirty |= r;
	    child = NEXT_CHILD(child);
	} else {
	    ASSERT(member != NULL);
	    if(is_geo_in_areas(member, n_areas, areas)) {
		draw_shape(rdman, canvas, member->shape);
		dirty = 1;
	    }
	    member = NEXT_MEMBER(member);
	    mem_idx++;
	}
    }

    if(dirty && coord->flags & COF_OWN_CANVAS) {
	update_canvas_2_parent(rdman, coord);
	clean_canvas_black(coord->canvas);
    }

    return dirty;
}

static void draw_shapes_in_areas(redraw_man_t *rdman,
				 int n_areas,
				 area_t **areas) {
    draw_coord_shapes_in_areas(rdman, rdman->root_coord, n_areas, areas);
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
    int r;
    int n_dirty_areas;
    area_t **dirty_areas;

    r = clean_rdman_dirties(rdman);
    if(r != OK)
	return ERR;

    n_dirty_areas = rdman->n_dirty_areas;
    dirty_areas = rdman->dirty_areas;
    if(n_dirty_areas > 0) {
	/*! \brief Draw shapes in preorder of coord tree and support opacity
	 * rules.
	 */
	clean_canvas(rdman->cr);
	draw_shapes_in_areas(rdman, n_dirty_areas, dirty_areas);
	copy_cr_2_backend(rdman, rdman->n_dirty_areas, rdman->dirty_areas);
	rdman->n_dirty_areas = 0;
	reset_clip(rdman);
    }
    rdman->n_dirty_areas = 0;

    return OK;
}

/* NOTE: Before redrawing, the canvas/surface must be cleaned.
 * NOTE: After redrawing, the content must be copied to the backend surface.
 */

/*! \page redraw How to Redraw Shapes?
 *
 * Coords are corresponding objects for group tags of SVG files.
 * In conceptional, every SVG group has a canvas, graphics of child shapes
 * are drawed into the canvas, applied filters of group, and blended into
 * canvas of parent of the group.
 *
 * But, we don't need to create actually a surface/canvas for every coord.
 * We only create surface for coords their opacity value are not 1 or they
 * apply filters on background.  Child shapes of coords without canvas
 * are drawed on canvas of nearest ancestor which have canvas.  It said
 * a coord owns a canvas or inherits from an ancestor. (\ref COF_OWN_CANVAS,
 * clean_coord()) Except, root_coord always owns a canvas.
 *
 * \note Default opacity of a coord is 1.
 *
 * \sa
 * - rdman_redraw_all()
 * - rdman_redraw_changed()
 * = draw_shapes_in_areas()
 */

int rdman_redraw_all(redraw_man_t *rdman) {
    cairo_surface_t *surface;
    area_t area;
    int r;

    area.x = area.y = 0;
#ifndef UNITTEST
    surface = cairo_get_target(rdman->cr);
    area.w = cairo_image_surface_get_width(surface);
    area.h = cairo_image_surface_get_height(surface);
#else
    area.w = 1024;
    area.h = 1024;
#endif
    add_dirty_area(rdman, &area);

    r = rdman_redraw_changed(rdman);
    if(r != OK)
	return ERR;

    return OK;
}

int rdman_redraw_area(redraw_man_t *rdman, co_aix x, co_aix y,
		      co_aix w, co_aix h) {
    area_t area;
    int r;

    area.x = x;
    area.y = y;
    area.w = w;
    area.h = h;
    add_dirty_area(rdman, &area);

    r = rdman_redraw_changed(rdman);

    return r;
}

geo_t *rdman_geos(redraw_man_t *rdman, geo_t *last) {
    geo_t *next;
    coord_t *coord;
    
    if(last == NULL) {
	coord = rdman->root_coord;
	while(coord != NULL && FIRST_MEMBER(coord) == NULL)
	    coord = preorder_coord_subtree(rdman->root_coord, coord);
	if(coord == NULL)
	    return NULL;
	return FIRST_MEMBER(coord);
    }

    coord = last->shape->coord;
    next = NEXT_MEMBER(last);
    while(next == NULL) {
	coord = preorder_coord_subtree(rdman->root_coord, coord);
	if(coord == NULL)
	    return NULL;
	next = FIRST_MEMBER(coord);
    }
    return next;
}

int rdman_force_clean(redraw_man_t *rdman) {
    int r;

    r = clean_rdman_dirties(rdman);

    return r;
}

shnode_t *shnode_new(redraw_man_t *rdman, shape_t *shape) {
    shnode_t *node;

    node = (shnode_t *)elmpool_elm_alloc(rdman->shnode_pool);
    if(node) {
	node->shape = shape;
	node->next = NULL;
    }
    return node;
}

/*! \page dirty Dirty geo, coord, and area.
 *
 * \section dirty_of_ego Dirty of geo
 * A geo is dirty when any of the shape, size or positions is changed.
 * It's geo and positions should be recomputed before drawing.  So,
 * dirty geos are marked as dirty and put into dirty_geos list.
 * The list is inspected before drawing to make sure the right shape,
 * size, and positions.
 *
 * \section dirty_of_coord Dirty of coord
 * A coord is dirty when it's transformation matrix being changed.
 * Dirty coords are marked as dirty and put into dirty_coords list.
 * Once a coord is dirty, every member geos of it are also dirty.
 * Because, their shape, size and positions will be changed.  But,
 * they are not marked as dirty and put into dirty_geos list, since
 * all these member geos will be recomputed for computing new current
 * area of the coord.  The changes of a coord also affect child
 * coords.  Once parent is dirty, all children are also dirty for
 * their aggregate matrix out of date.  Dirty coords should be
 * clean in preorder of tree traversal.  The dirty_coords list
 * are sorted to keep the order before cleaning.
 * Whenever a coord is marked dirty and put into dirty_coords list,
 * all it's children should also be marked and put.
 *
 * The procedure of clean coords comprises recomputing aggregate
 * tranform matrix and area where members spreading in.
 *
 * The list is inspected before drawing to recompute new shape, size,
 * and positions of member geos of coords in the list.  The drity
 * flag of member geos will be clean.
 *
 * Clean coords should be performed before clean geos, since clean
 * coords will also clean member geos.
 */

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

/* Implment factory and strategy functions for observers and subjects.
 */
static subject_t *ob_subject_alloc(ob_factory_t *factory) {
    redraw_man_t *rdman;
    subject_t *subject;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    subject = elmpool_elm_alloc(rdman->subject_pool);

    return subject;
}

static void ob_subject_free(ob_factory_t *factory, subject_t *subject) {
    redraw_man_t *rdman;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    elmpool_elm_free(rdman->subject_pool, subject);
}

static observer_t *ob_observer_alloc(ob_factory_t *factory) {
    redraw_man_t *rdman;
    observer_t *observer;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    observer = elmpool_elm_alloc(rdman->observer_pool);

    return observer;
}

static void ob_observer_free(ob_factory_t *factory, observer_t *observer) {
    redraw_man_t *rdman;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    elmpool_elm_free(rdman->observer_pool, observer);
}

static subject_t *ob_get_parent_subject(ob_factory_t *factory,
					subject_t *cur_subject) {
    redraw_man_t *rdman;
    coord_t *coord, *parent_coord;
    geo_t *geo;
    subject_t *parent;

    rdman = MEM2OBJ(factory, redraw_man_t, ob_factory);
    switch(cur_subject->obj_type) {
    case OBJT_GEO:
	geo = (geo_t *)cur_subject->obj;
	parent_coord = geo->shape->coord;
	parent = parent_coord->mouse_event;
	break;
    case OBJT_COORD:
	coord = (coord_t *)cur_subject->obj;
	parent_coord = coord->parent;
	if(parent_coord == NULL) {
	    parent = NULL;
	    break;
	}
	parent = parent_coord->mouse_event;
	break;
    default:
	parent = NULL;
	break;
    }

    return parent;
}

#ifdef UNITTEST
/* Test cases */

#include <CUnit/Basic.h>
#include "paint.h"

struct _sh_dummy {
    shape_t shape;
    co_aix x, y;
    co_aix w, h;
    int trans_cnt;
    int draw_cnt;
};

shape_t *sh_dummy_new(co_aix x, co_aix y, co_aix w, co_aix h) {
    sh_dummy_t *dummy;

    dummy = (sh_dummy_t *)malloc(sizeof(sh_dummy_t));
    if(dummy == NULL)
	return NULL;

    memset(dummy, 0, sizeof(sh_dummy_t));

    dummy->x = x;
    dummy->y = y;
    dummy->w = w;
    dummy->h = h;
    dummy->trans_cnt = 0;
    dummy->draw_cnt = 0;

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
    
	if(shape->geo)
	    geo_from_positions(shape->geo, 2, poses);
    }
    dummy->trans_cnt++;
}

void sh_dummy_fill(shape_t *shape, cairo_t *cr) {
    sh_dummy_t *dummy;

    dummy = (sh_dummy_t *)shape;
    dummy->draw_cnt++;
}

static void dummy_paint_prepare(paint_t *paint, cairo_t *cr) {
}

static void dummy_paint_free(paint_t *paint) {
    if(paint)
	free(paint);
}

paint_t *dummy_paint_new(redraw_man_t *rdman) {
    paint_t *paint;

    paint = (paint_t *)malloc(sizeof(paint_t));
    if(paint == NULL)
	return NULL;

    paint_init(paint, dummy_paint_prepare, dummy_paint_free);

    return paint;
}

void test_rdman_redraw_changed(void) {
    coord_t *coords[3];
    shape_t *shapes[3];
    sh_dummy_t **dummys;
    paint_t *paint;
    redraw_man_t *rdman;
    redraw_man_t _rdman;
    int i;

    dummys = (sh_dummy_t **)shapes;

    rdman = &_rdman;
    redraw_man_init(rdman, NULL, NULL);
    paint = dummy_paint_new(rdman);
    for(i = 0; i < 3; i++) {
	shapes[i] = sh_dummy_new(0, 0, 50, 50);
	rdman_paint_fill(rdman, paint, shapes[i]);
	coords[i] = rdman_coord_new(rdman, rdman->root_coord);
	coords[i]->matrix[2] = 10 + i * 100;
	coords[i]->matrix[5] = 10 + i * 100;
	rdman_coord_changed(rdman, coords[i]);
	rdman_add_shape(rdman, shapes[i], coords[i]);
    }
    rdman_redraw_all(rdman);
    CU_ASSERT(dummys[0]->trans_cnt == 1);
    CU_ASSERT(dummys[1]->trans_cnt == 1);
    CU_ASSERT(dummys[2]->trans_cnt == 1);
    CU_ASSERT(dummys[0]->draw_cnt == 1);
    CU_ASSERT(dummys[1]->draw_cnt == 1);
    CU_ASSERT(dummys[2]->draw_cnt == 1);
    
    coords[2]->matrix[2] = 100;
    coords[2]->matrix[5] = 100;
    rdman_coord_changed(rdman, coords[0]);
    rdman_coord_changed(rdman, coords[2]);
    rdman_redraw_changed(rdman);

    CU_ASSERT(dummys[0]->draw_cnt == 2);
    CU_ASSERT(dummys[1]->draw_cnt == 2);
    CU_ASSERT(dummys[2]->draw_cnt == 2);

    paint->free(paint);
    redraw_man_destroy(rdman);
}

CU_pSuite get_redraw_man_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_redraw_man", NULL, NULL);
    CU_ADD_TEST(suite, test_rdman_redraw_changed);

    return suite;
}

#endif /* UNITTEST */
