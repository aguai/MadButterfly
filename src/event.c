/*! \file
 * \brief Convenience functions for event relative work.
 */
#include <stdio.h>
#include <stdlib.h>
#ifndef UNITTEST
#include <cairo.h>
#include "mb_types.h"
#include "mb_redraw_man.h"
#include "mb_shapes.h"
#endif

#define OK 0
#define ERR -1
#define FALSE 0
#define TRUE 1

#define ARRAY_EXT_SZ 64

#define ASSERT(x)

#ifdef UNITTEST
/* ============================================================ */
#include <string.h>
#include "mb_tools.h"

typedef float co_aix;

typedef struct shape shape_t;
typedef struct cairo_surface cairo_surface_t;
typedef struct coord coord_t;

typedef struct cairo cairo_t;
struct cairo {
    STAILQ(shape_t) drawed;
    STAILQ(shape_t) clip_pathes;
    cairo_surface_t *tgt;
};

struct cairo_surface {
    cairo_t *cr;
    int w, h;
    unsigned char *data;
};

#define cairo_new_path(cr) do { STAILQ_CLEAN((cr)->drawed); } while(0)
#define cairo_get_target(cr) (cr)->tgt
static
cairo_t *cairo_create(cairo_surface_t *target) {
    cairo_t *cr;

    cr = (cairo_t *)malloc(sizeof(cairo_t));
    STAILQ_INIT(cr->drawed);
    STAILQ_INIT(cr->clip_pathes);
    cr->tgt = target;
    target->cr = cr;

    return cr;
}
#define cairo_destroy(cr) do { free(cr); } while(0)
#define cairo_clip(cr)			\
    do {				\
	memcpy(&(cr)->clip_pathes,	\
	       &(cr)->drawed,		\
	       sizeof((cr)->drawed));	\
	STAILQ_CLEAN((cr)->drawed);	\
    } while(0)
#define cairo_fill(cr)

#define cairo_image_surface_get_width(surface) (surface)->w
#define cairo_image_surface_get_height(surface) (surface)->h
static
cairo_surface_t *cairo_image_surface_create(int format, int w, int h) {
    cairo_surface_t *surf;

    surf = (cairo_surface_t *)malloc(sizeof(cairo_surface_t));
    surf->w = w;
    surf->h = h;
    surf->data = (unsigned char *)malloc(h);
    memset(surf->data, 0, h);

    return surf;
}
#define cairo_surface_destroy(surface)		\
    do { free((surface)->data); free(surface); } while(0)
#define cairo_image_surface_get_stride(surface) 1
#define CAIRO_FORMAT_A1 1


typedef struct _area area_t;
struct _area {
    co_aix x, y;
    co_aix w, h;
};
#define area_set(area, _x, _y, _w, _h)		\
    do {					\
	(area)->x = (_x);			\
	(area)->y = (_y);			\
	(area)->w = (_w);			\
	(area)->h = (_h);			\
    } while(0)
#define _in_range(a, s, w) ((a) >= (s) && (a) < ((s) + (w)))
#define _range_overlay(as, aw, bs, bw)			\
    (_in_range(as, bs, bw) || _in_range(bs, as, aw))
#define areas_are_overlay(a1, a2)		\
    (_range_overlay((a1)->x, (a1)->w,		\
		    (a2)->x, (a2)->w) &&	\
     _range_overlay((a1)->y, (a1)->h,		\
		    (a2)->y, (a2)->h))
#define area_pos_is_in(area, _x, _y)		\
    (_in_range(_x, (area)->x, (area)->w) &&	\
     _in_range(_y, (area)->y, (area)->h))
#define _range_extent(a, s, w)			\
    do {					\
	if((a) < (s)) {				\
	    (w) += (s) - (a);			\
	    (s) = (a);				\
	} else {				\
	    (w) = MAX(w, (a) - (s) + 1);	\
	}					\
    } while(0)

static
void area_extent(area_t *area, co_aix x, co_aix y) {
    _range_extent(x, area->x, area->w);
    _range_extent(y, area->y, area->h);
}

struct mb_obj {
    int obj_type;
};
typedef struct mb_obj mb_obj_t;
#define MB_OBJ_INIT(obj, type) do { (obj)->obj_type = type; } while(0)

#define GEF_OV_DRAW 0x1
#define GEF_HIDDEN 0x2

struct shape {
    mb_obj_t obj;
    
    coord_t *coord;
    area_t area;
    shape_t *all_next;
    shape_t *drawed_next;

    void *fill, *stroke;
    struct shape *sibling;
    int flags;
    
    int num_points;
    co_aix points[32][2];
};
enum { MBO_DUMMY,
       MBO_COORD,
       MBO_SHAPES=0x1000,
       MBO_PATH,
       MBO_TEXT,
       MBO_RECT
};
#define MBO_TYPE(x) (((mb_obj_t *)(x))->obj_type)
#define IS_MBO_SHAPES(x) (((mb_obj_t *)(x))->obj_type & MBO_SHAPES)
#define sh_get_geo(x) ((x)->geo)
static int sh_pos_is_in(shape_t *shape, co_aix x, co_aix y) {
    int i;

    for(i = 0; i < shape->num_points; i++)
	if(shape->points[i][0] == x && shape->points[i][1] == y)
	    return TRUE;
    return FALSE;
}
#define sh_get_flags(shape, mask) ((shape)->flags & mask)
#define sh_set_flags(shape, mask) do { (shape)->flags |= mask; } while(0)
#define sh_clear_flags(shape, mask) do { (shape)->flags &= ~(mask); } while(0)
#define sh_get_area(shape) (&(shape)->area)

struct coord {
    mb_obj_t obj;
  
    area_t area;
    int flags;
    coord_t *parent;
    STAILQ(coord_t) children;
    coord_t *sibling;
    STAILQ(shape_t) shapes;
};

#define COF_SKIP 0x1

#define coord_get_area(coord) (&(coord)->area)
#define FOR_COORD_SHAPES(coord, shape)		\
    for((shape) = STAILQ_HEAD((coord)->shapes);		\
	(shape) != NULL;				\
	(shape) = STAILQ_NEXT(shape_t, sibling, shape))
#define FOR_COORDS_PREORDER(root, last)			\
    for((last) = (root);				\
	(last) != NULL;					\
	(last) = preorder_coord_subtree(root, last))
#define FOR_COORD_CHILDREN(parent, child)		\
    for((child) = STAILQ_HEAD((parent)->children);	\
	(child) != NULL;				\
	(child) = STAILQ_NEXT(coord_t, sibling, child))

static
void _areas_merge(area_t *area1, area_t *area2) {
    co_aix lu_x, lu_y;
    co_aix rb_x, rb_y;
    
    lu_x = area2->x;
    lu_y = area2->y;
    rb_x = lu_x + area2->w - 1;
    rb_y = lu_y + area2->h - 1;
    area_extent(area1, lu_x, lu_y);
    area_extent(area1, rb_x, rb_y);
}

static
void coord_update_area(coord_t *coord) {
    area_t *area;
    shape_t *shape;
    coord_t *child;
    area_t *cur_area;

    area = coord_get_area(coord);
    
    shape = STAILQ_HEAD(coord->shapes);
    if(shape != NULL) {
	cur_area = sh_get_area(shape);
    } else {
	child = STAILQ_HEAD(coord->children);
	if(child == NULL)
	    return;
	cur_area = coord_get_area(child);
    }
    memcpy(area, cur_area, sizeof(area_t));
	
    FOR_COORD_SHAPES(coord, shape) {
	cur_area = sh_get_area(shape);
	_areas_merge(area, cur_area);
    }

    FOR_COORD_CHILDREN(coord, child) {
	cur_area = coord_get_area(child);
	_areas_merge(area, cur_area);
    }
}

static
void coord_update_area_ancestors(coord_t *coord) {
    coord_t *cur;

    for(cur = coord; cur != NULL; cur = cur->parent) {
	coord_update_area(cur);
    }
}

static
coord_t *preorder_coord_subtree(coord_t *root, coord_t *last) {
    if(STAILQ_HEAD(last->children) && !(last->flags & COF_SKIP))
	return STAILQ_HEAD(last->children);
    if(last == root)
	return NULL;
    while(STAILQ_NEXT(coord_t, sibling, last) == NULL) {
	if(last == root)
	    return NULL;
	last = last->parent;
    }
    return STAILQ_NEXT(coord_t, sibling, last);
}

static
void preorder_coord_skip_subtree(coord_t *coord) {
    coord->flags &= ~COF_SKIP;
}

static
coord_t *postorder_coord_subtree(coord_t *root, coord_t *last) {
    coord_t *cur;
    
    if(last != NULL) {
	if(STAILQ_NEXT(coord_t, sibling, last) == NULL) {
	    if(cur == root)
		return NULL;
	    cur = last->parent;
	    return cur;
	}
	cur = STAILQ_NEXT(coord_t, sibling, last);
    }

    cur = root;
    while(STAILQ_HEAD(cur->children)) {
	cur = STAILQ_HEAD(cur->children);
    }
    return cur;
}

static
void shape_draw(shape_t *sh, cairo_t *cr) {
    STAILQ_INS_TAIL(cr->drawed, shape_t, drawed_next, sh);
}

#define sh_path_draw(path, cr) shape_draw((shape_t *)path, cr)
#define sh_text_draw(text, cr) shape_draw((shape_t *)text, cr)
#define sh_rect_draw(rect, cr) shape_draw((shape_t *)rect, cr)
static
void sh_update_area(shape_t *sh) {
    int i;
    co_aix x, y;
    area_t *area = &sh->area;

    if(sh->num_points == 0) {
	area_set(area, 0, 0, 0, 0);
	return;
    }

    area_set(area, sh->points[0][0], sh->points[0][1], 1, 1);
    for(i = 1; i < sh->num_points; i++) {
	x = sh->points[i][0];
	y = sh->points[i][1];
	area_extent(area, x, y);
    }
}


struct redraw_man {
    cairo_t *cr;
    coord_t *root_coord;
    int shape_gl_sz;
    shape_t *shape_gl[32];
    STAILQ(shape_t) all_shapes;
};
typedef struct redraw_man redraw_man_t;
#define rdman_get_cr(rdman) ((rdman)->cr)
#define rdman_force_clean(rdman) OK
#define rdman_clear_shape_gl(rdman) do {(rdman)->shape_gl_sz = 0; } while(0)
static int rdman_add_shape_gl(redraw_man_t *rdman, shape_t *shape) {
    (rdman)->shape_gl[(rdman)->shape_gl_sz++] = shape;
    return OK;
}
#define rdman_get_shape_gl(rdman, idx)		\
    (rdman)->shape_gl[idx]
#define rdman_shape_gl_len(rdman) (rdman)->shape_gl_sz
static shape_t *rdman_shapes(redraw_man_t *rdman, shape_t *last_shape) {
    if(last_shape == NULL)
	return STAILQ_HEAD(rdman->all_shapes);

    return STAILQ_NEXT(shape_t, all_next, last_shape);
}
#define redraw_man_init(rdman, cr, backend)			\
    do {							\
	memset(rdman, 0, sizeof(redraw_man_t));			\
	(rdman)->cr = cr;					\
	(rdman)->root_coord = rdman_coord_new_noparent(rdman);	\
    } while(0)
#define redraw_man_destroy(rdman)			\
    do {						\
	free(rdman);					\
    } while(0)
#define rdman_get_root(rdman) ((rdman)->root_coord)

static coord_t *rdman_coord_new_noparent(redraw_man_t *rdman);

static
redraw_man_t *redraw_man_new(cairo_t *cr, cairo_t *backend) {
    redraw_man_t *rdman;

    rdman = O_ALLOC(redraw_man_t);
    redraw_man_init(rdman, cr, backend);
    return rdman;
}
#define redraw_man_free(rdman)			\
    do {					\
	redraw_man_destroy(rdman);		\
	free(rdman);				\
    } while(0)

static
int cairo_in_fill(cairo_t *cr, int x, int y) {
    shape_t *shape;
    int i;

    for(shape = STAILQ_HEAD(cr->drawed);
	shape != NULL;
	shape = STAILQ_NEXT(shape_t, drawed_next, shape)) {
	for(i = 0; i < shape->num_points; i++)
	    if(shape->points[i][0] == x &&
	       shape->points[i][1] == y)
		return 1;
    }
    return 0;
}

#define cairo_in_stroke cairo_in_fill

static
void rdman_coord_init_noparent(redraw_man_t *rdman, coord_t *co) {
    memset(co, 0, sizeof(coord_t));
    MB_OBJ_INIT(&co->obj, MBO_COORD);
    STAILQ_INIT(co->children);
    STAILQ_INIT(co->shapes);
}

static
void rdman_coord_init(redraw_man_t *rdman, coord_t *co, coord_t *parent) {
    rdman_coord_init_noparent(rdman, co);
    STAILQ_INS_TAIL(parent->children, coord_t, sibling, co);
    co->parent = parent;
}

static
coord_t *rdman_coord_new(redraw_man_t *rdman, coord_t *parent) {
    coord_t *coord;

    coord = O_ALLOC(coord_t);
    rdman_coord_init(rdman, coord, parent);

    return coord;
}

coord_t *rdman_coord_new_noparent(redraw_man_t *rdman) {
    coord_t *coord;

    coord = O_ALLOC(coord_t);
    rdman_coord_init_noparent(rdman, coord);

    return coord;
}

static
void rdman_coord_free(redraw_man_t *rdman, coord_t *coord) {
    free(coord);
}

static
shape_t *rdman_shape_new(redraw_man_t *rdman) {
    shape_t *shape;

    shape = O_ALLOC(shape_t);
    memset(shape, 0, sizeof(shape_t));
    MB_OBJ_INIT(&shape->obj, MBO_PATH);
    STAILQ_INS(rdman->all_shapes, shape_t, all_next, shape);
    
    return shape;
}

static
void rdman_shape_free(redraw_man_t *rdman, shape_t *shape) {
    STAILQ_REMOVE(rdman->all_shapes, shape_t, all_next, shape);
    free(shape);
}

#define shape_add_point(shape, x, y)				\
    do {							\
	(shape)->points[(shape)->num_points][0] = x;		\
	(shape)->points[(shape)->num_points][1] = y;		\
	(shape)->num_points++;					\
	sh_update_area(shape);					\
	if((shape)->coord)					\
	    coord_update_area_ancestors((shape)->coord);	\
    } while(0)

static
int rdman_add_shape(redraw_man_t *rdman, shape_t *shape,
		     coord_t *parent) {
    STAILQ_INS_TAIL(parent->shapes, shape_t, sibling, shape);
    shape->coord = parent;

    return OK;
}

static
void *cairo_image_surface_get_data(cairo_surface_t *surf) {
    cairo_t *cr;
    shape_t *shape1, *shape2;
    co_aix x1, y1, x2, y2;
    int i, j;

    cr = surf->cr;
    
    STAILQ_FOR_EACH(cr->drawed, shape_t, sibling, shape1) {
	for(i = 0; i < shape1->num_points; i++) {
	    x1 = shape1->points[i][0];
	    y1 = shape1->points[i][1];
	    STAILQ_FOR_EACH(cr->clip_pathes, shape_t, sibling, shape2) {
		for(j = 0; j < shape2->num_points; j++) {
		    x2 = shape2->points[j][0];
		    y2 = shape2->points[j][1];
		    if(x1 == x2 && y1 == y2) {
			surf->data[0] = 1;
			return surf->data;
		    }
		}
	    }
	}
    }
    surf->data[0] = 0;
    return surf->data;
}
/* ============================================================ */
#endif /* UNITTEST */


static int _collect_shapes_at_point(redraw_man_t *rdman,
				   co_aix x, co_aix y) {
    shape_t *shape;
    int r;
    
    r = rdman_force_clean(rdman);
    if(r != OK)
	return ERR;

    rdman_clear_shape_gl(rdman);

    for(shape = rdman_shapes(rdman, (shape_t *)NULL);
	shape != NULL;
	shape = rdman_shapes(rdman, shape)) {
	if(sh_pos_is_in(shape, x, y)) {
	    r = rdman_add_shape_gl(rdman, shape);
	    if(r != 0)
		return ERR;
	}
    }

    return OK;
}

static void draw_shape_path(shape_t *shape, cairo_t *cr) {
    switch(MBO_TYPE(shape)) {
    case MBO_PATH:
	sh_path_draw(shape, cr);
	break;
    case MBO_TEXT:
	sh_text_draw(shape, cr);
	break;
    case MBO_RECT:
	sh_rect_draw(shape, cr);
	break;
    case MBO_IMAGE:
	sh_image_draw(shape, cr);
	break;
    }
}

static int _shape_pos_is_in_cairo(shape_t *shape, co_aix x, co_aix y,
				  int *in_stroke, cairo_t *cr) {
    draw_shape_path(shape, cr);
    if(shape->fill) {
	if(cairo_in_fill(cr, x, y)) {
	    *in_stroke = 0;
	    return TRUE;
	}
    }
    if(shape->stroke) {
	if(cairo_in_stroke(cr, x, y)) {
	    *in_stroke = 1;
	    return TRUE;
	}
    }
    return FALSE;
}

static
int _shape_pos_is_in(shape_t *shape, co_aix x, co_aix y,
		     int *in_stroke, cairo_t *cr) {
    int r;

    r = sh_pos_is_in(shape, x, y);
    if(!r)
	return FALSE;

    r = _shape_pos_is_in_cairo(shape, x, y, in_stroke, cr);
    cairo_new_path(cr);
    if(!r)
	return FALSE;

    return TRUE;
}

static shape_t *_find_shape_in_pos(redraw_man_t *rdman,
				   co_aix x, co_aix y, int *in_stroke) {
    shape_t *shape;
    cairo_t *cr;
    int i, r;

    cr = rdman_get_cr(rdman);
    for(i = rdman_shape_gl_len(rdman) - 1; i >= 0; i--) {
	shape = rdman_get_shape_gl(rdman, i);
	if(sh_get_flags(shape, GEF_HIDDEN))
	    continue;
	r = _shape_pos_is_in_cairo(shape, x, y, in_stroke, cr);
	if(r)
	    return shape;
    }

    return NULL;
}

shape_t *find_shape_at_pos(redraw_man_t *rdman,
			   co_aix x, co_aix y, int *in_stroke) {
    shape_t *shape;
    int r;

    r = _collect_shapes_at_point(rdman, x, y);
    if(r != OK)
	return NULL;

    shape = _find_shape_in_pos(rdman, x, y, in_stroke);
    return shape;
}

/*! \brief Test if an object and descendants cover the position
 *	specified by x,y.
 *
 * \param in_stroke is x, y is on a stroke.
 */
int mb_obj_pos_is_in(redraw_man_t *rdman, mb_obj_t *obj,
		     co_aix x, co_aix y, int *in_stroke) {
    coord_t *cur_coord, *root;
    shape_t *shape;
    area_t *area;
    int r;

    if(IS_MBO_SHAPES(obj)) {
	shape = (shape_t *)obj;
	r = _shape_pos_is_in(shape, x, y, in_stroke, rdman_get_cr(rdman));
	return r;
    }
    root = (coord_t *)obj;
    FOR_COORDS_PREORDER(root, cur_coord) {
	area = coord_get_area(cur_coord);
	if(!area_pos_is_in(area, x, y)) {
	    preorder_coord_skip_subtree(cur_coord);
	    continue;
	}
	FOR_COORD_SHAPES(cur_coord, shape) {
	    r = _shape_pos_is_in(shape, x, y, in_stroke, rdman_get_cr(rdman));
	    if(r)
		return TRUE;
	}
    }
    return FALSE;
}

static
cairo_t * _prepare_cairo_for_testing(redraw_man_t *rdman) {
    cairo_surface_t *surface, *rdman_surface;
    cairo_t *cr;
    int w, h;

    rdman_surface = cairo_get_target(rdman_get_cr(rdman));
    w = cairo_image_surface_get_width(rdman_surface);
    h = cairo_image_surface_get_height(rdman_surface);
    
    surface = cairo_image_surface_create(CAIRO_FORMAT_A1, w, h);
    if(surface == NULL)
	return NULL;

    cr = cairo_create(surface);
    if(cr == NULL)
	cairo_surface_destroy(surface);
    
    return cr;
}

static
void _release_cairo_for_testing(cairo_t *cr) {
    cairo_destroy(cr);
}

static
void _draw_to_mask(shape_t *shape, cairo_t *cr) {
    if(sh_get_flags(shape, GEF_OV_DRAW))
	return;
    
    draw_shape_path(shape, cr);
    cairo_clip(cr);
    
    sh_set_flags(shape, GEF_OV_DRAW);
}

static
int _fill_and_check(shape_t *shape, cairo_t *cr) {
    int h, stride;
    cairo_surface_t *surface;
    unsigned char *data;
    int i, sz;

    draw_shape_path(shape, cr);
    cairo_fill(cr);

    surface = cairo_get_target(cr);
    data = cairo_image_surface_get_data(surface);

    h = cairo_image_surface_get_height(surface);
    stride = cairo_image_surface_get_stride(surface);

    sz = stride * h;
    for(i = 0; i < sz; i++) {
	if(data[i])
	    return TRUE;
    }

    return FALSE;
}

/*! \brief Is a mb_obj_t overlaid with another mb_obj_t and
 *	descendants.
 *
 * coord is relative less than shapes.  Check areas of coord can
 * skip sub-trees and avoid useless heavy computation.  For shapes,
 * it not only check overlay of area.  It also check overlay by
 * actually drawing on a cairo surface.
 */
static
int _is_obj_objs_overlay(mb_obj_t *obj, mb_obj_t *others_root,
			 cairo_t *cr) {
    area_t *area, *candi_area;
    coord_t *coord, *candi_coord, *root;
    shape_t *shape, *candi_shape;
    int obj_is_shape;
    int r;
    
    obj_is_shape = IS_MBO_SHAPES(obj);
    
    if(obj_is_shape) {
	shape = (shape_t *)obj;
	area = sh_get_area(shape);
    } else {
	coord = (coord_t *)obj;
	area = coord_get_area(coord);
	shape = NULL;
    }
	
    if(IS_MBO_SHAPES(others_root)) {
	candi_shape = (shape_t *)others_root;
	candi_area =  sh_get_area(candi_shape);
	
	r = areas_are_overlay(area, candi_area);
	if(!r)
	    return FALSE;
	
	if(!obj_is_shape)
	    return TRUE;
	
	_draw_to_mask(candi_shape, cr);
	r = _fill_and_check(shape, cr);
	
	return r;
    }
    
    ASSERT(IS_MBO_COORD(others_root));
    
    root = (coord_t *)others_root;
    FOR_COORDS_PREORDER(root, candi_coord) {
	candi_area = coord_get_area(candi_coord);
	r = areas_are_overlay(area, candi_area);
	if(!r) {
	    preorder_coord_skip_subtree(candi_coord);
	    continue;
	}
	
	FOR_COORD_SHAPES(candi_coord, candi_shape) {
	    candi_area = sh_get_area(candi_shape);
	    r = areas_are_overlay(area, candi_area);
	    if(!r)
		continue;
	    
	    if(!obj_is_shape)
		return TRUE;
	    
	    _draw_to_mask(candi_shape, cr);
	    r = _fill_and_check(shape, cr);
	    if(r)
		return TRUE;
	}
    }
    
    return FALSE;
}

static
void _clear_ov_draw(mb_obj_t *obj) {
    coord_t *coord, *root;
    shape_t *shape;

    if(IS_MBO_SHAPES(obj)) {
	shape = (shape_t *)obj;
	sh_clear_flags(shape, GEF_OV_DRAW);
	return;
    }

    root = (coord_t *)obj;
    FOR_COORDS_PREORDER(root, coord) {
	FOR_COORD_SHAPES(coord, shape) {
	    sh_clear_flags(shape, GEF_OV_DRAW);
	}
    }
}

/*! \brief Test if two objects are overlaid.
 *
 * \todo Detect overlay in better way with cairo.
 * \note This function cost heavy on CPU power.
 */
int mb_objs_are_overlay(redraw_man_t *rdman,
		       mb_obj_t *obj1, mb_obj_t *obj2) {
    cairo_t *cr;
    area_t *area;
    shape_t *shape;
    coord_t *coord, *root;
    int r;

    cr = _prepare_cairo_for_testing(rdman);

    if(IS_MBO_SHAPES(obj1)) {
	shape = (shape_t *)obj1;
	r = _is_obj_objs_overlay(obj1, obj2, cr);
	goto out;
    }
    
    root = (coord_t *)obj1;
    FOR_COORDS_PREORDER(root, coord) {
	area = coord_get_area(coord);
	r = _is_obj_objs_overlay((mb_obj_t *)coord, obj2, cr);
	if(!r) {
	    preorder_coord_skip_subtree(coord);
	    continue;
	}

	FOR_COORD_SHAPES(coord, shape) {
	    r = _is_obj_objs_overlay((mb_obj_t *)shape, obj2, cr);
	    if(r)
		goto out;
	}
    }
    r = FALSE;
    
 out:
    _clear_ov_draw(obj2);	/* marked by _is_obj_objs_overlay() */
    _release_cairo_for_testing(cr);
    return r;
}

#ifdef UNITTEST

#include <CUnit/Basic.h>

static
redraw_man_t *_fake_rdman(void) {
    redraw_man_t *rdman;
    cairo_t *cr, *backend;
    cairo_surface_t *surf;

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    backend = cairo_create(surf);
    rdman = redraw_man_new(cr, backend);
    
    return rdman;
}

static
void _free_fake_rdman(redraw_man_t *rdman) {
    cairo_surface_destroy(rdman->cr->tgt);
    cairo_destroy(rdman->cr);
    free(rdman);
}

static
void test_mb_obj_pos_is_in(void) {
    redraw_man_t *rdman;
    shape_t *shape;
    coord_t *root, *child_coord;
    int in_stroke = 0;
    int r;

    rdman = _fake_rdman();
    CU_ASSERT(rdman != NULL);

    root = rdman_get_root(rdman);

    child_coord = rdman_coord_new(rdman, root);
    CU_ASSERT(child_coord != NULL);

    shape = rdman_shape_new(rdman);
    CU_ASSERT(shape != NULL);

    rdman_add_shape(rdman, shape, child_coord);

    shape_add_point(shape, 3, 12);

    shape->fill = shape;
    shape->stroke = shape;

    r = mb_obj_pos_is_in(rdman, (mb_obj_t *)shape, 3, 12, &in_stroke);
    CU_ASSERT(r == TRUE);

    r = mb_obj_pos_is_in(rdman, (mb_obj_t *)shape, 3, 13, &in_stroke);
    CU_ASSERT(r == FALSE);

    r  = mb_obj_pos_is_in(rdman, (mb_obj_t *)root, 3, 12, &in_stroke);
    CU_ASSERT(r == TRUE);

    r  = mb_obj_pos_is_in(rdman, (mb_obj_t *)root, 4, 12, &in_stroke);
    CU_ASSERT(r == FALSE);

    rdman_shape_free(rdman, shape);
    _free_fake_rdman(rdman);
}

static
void test_is_obj_objs_overlay(void) {
    redraw_man_t *rdman;
    coord_t *root, *coord1, *coord2;
    shape_t *shape1, *shape2, *shape3;
    cairo_t *cr;
    cairo_surface_t *surf;
    int r;

    rdman = _fake_rdman();
    CU_ASSERT(rdman != NULL);

    root = rdman_get_root(rdman);

    coord1 = rdman_coord_new(rdman, root);
    shape1 = rdman_shape_new(rdman);
    rdman_add_shape(rdman, shape1, coord1);

    coord2 = rdman_coord_new(rdman, root);
    shape2 = rdman_shape_new(rdman);
    rdman_add_shape(rdman, shape2, coord2);

    shape3 = rdman_shape_new(rdman);
    rdman_add_shape(rdman, shape3, coord2);

    shape_add_point(shape1, 3, 2);
    shape_add_point(shape2, 5, 5);
    shape_add_point(shape3, 4, 3);
    
    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)coord1, (mb_obj_t *)coord2, cr);
    CU_ASSERT(!r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(coord2, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)shape1, (mb_obj_t *)coord2, cr);
    CU_ASSERT(!r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(coord2, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)coord1, (mb_obj_t *)shape2, cr);
    CU_ASSERT(!r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(shape2, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)shape1, (mb_obj_t *)shape2, cr);
    CU_ASSERT(!r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(shape2, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)shape1, (mb_obj_t *)shape3, cr);
    CU_ASSERT(!r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(shape3, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)coord1, (mb_obj_t *)shape3, cr);
    CU_ASSERT(!r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(shape3, GEF_OV_DRAW);
    
    shape_add_point(shape1, 5, 5);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)coord1, (mb_obj_t *)coord2, cr);
    CU_ASSERT(r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(coord2, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)shape1, (mb_obj_t *)coord2, cr);
    CU_ASSERT(r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(coord2, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)coord1, (mb_obj_t *)shape2, cr);
    CU_ASSERT(r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(shape2, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)shape1, (mb_obj_t *)shape2, cr);
    CU_ASSERT(r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(shape2, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)shape1, (mb_obj_t *)shape3, cr);
    CU_ASSERT(!r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(shape3, GEF_OV_DRAW);

    surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    cr = cairo_create(surf);
    r = _is_obj_objs_overlay((mb_obj_t *)coord1, (mb_obj_t *)shape3, cr);
    CU_ASSERT(r);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    sh_clear_flags(shape3, GEF_OV_DRAW);

    rdman_shape_free(rdman, shape1);
    rdman_shape_free(rdman, shape2);
    rdman_shape_free(rdman, shape3);
    rdman_coord_free(rdman, coord1);
    rdman_coord_free(rdman, coord2);
    _free_fake_rdman(rdman);
}

static
void test_mb_objs_are_overlay(void) {
    redraw_man_t *rdman;
    coord_t *root, *coord1, *coord2;
    shape_t *shape1, *shape2, *shape3;
    int r;

    rdman = _fake_rdman();

    root = rdman_get_root(rdman);

    coord1 = rdman_coord_new(rdman, root);
    shape1 = rdman_shape_new(rdman);
    rdman_add_shape(rdman, shape1, coord1);

    coord2 = rdman_coord_new(rdman, root);
    shape2 = rdman_shape_new(rdman);
    rdman_add_shape(rdman, shape2, coord2);

    shape3 = rdman_shape_new(rdman);
    rdman_add_shape(rdman, shape3, coord2);

    shape_add_point(shape1, 3, 2);
    shape_add_point(shape2, 5, 5);
    shape_add_point(shape3, 4, 3);

    r = mb_objs_are_overlay(rdman, (mb_obj_t *)coord1, (mb_obj_t *)coord2);
    CU_ASSERT(!r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)shape1, (mb_obj_t *)coord2);
    CU_ASSERT(!r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)shape1, (mb_obj_t *)shape2);
    CU_ASSERT(!r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)coord1, (mb_obj_t *)shape2);
    CU_ASSERT(!r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)shape1, (mb_obj_t *)shape3);
    CU_ASSERT(!r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)coord1, (mb_obj_t *)shape3);
    CU_ASSERT(!r);

    shape_add_point(shape1, 5, 5);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)coord1, (mb_obj_t *)coord2);
    CU_ASSERT(r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)shape1, (mb_obj_t *)coord2);
    CU_ASSERT(r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)shape1, (mb_obj_t *)shape2);
    CU_ASSERT(r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)coord1, (mb_obj_t *)shape2);
    CU_ASSERT(r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)shape1, (mb_obj_t *)shape3);
    CU_ASSERT(!r);
    
    r = mb_objs_are_overlay(rdman, (mb_obj_t *)coord1, (mb_obj_t *)shape3);
    CU_ASSERT(!r);

    _free_fake_rdman(rdman);
}

CU_pSuite get_event_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_event", NULL, NULL);
    CU_ADD_TEST(suite, test_mb_obj_pos_is_in);
    CU_ADD_TEST(suite, test_is_obj_objs_overlay);
    CU_ADD_TEST(suite, test_mb_objs_are_overlay);

    return suite;
}

#endif /* UNITTEST */
