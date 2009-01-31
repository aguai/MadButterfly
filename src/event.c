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

typedef struct shape shape_t;

typedef struct cairo cairo_t;
struct cairo {
    shape_t *drawed;
};
#define cairo_in_fill(cr, x, y) 0
#define cairo_in_stroke(cr, x, y) 0
#define cairo_new_path(cr)
#define cairo_get_target(cr) NULL
#define cairo_create(target) NULL
#define cairo_destroy(cr)
#define cairo_clip(cr)
#define cairo_fill(cr)
#define cairo_image_surface_get_data(cr) NULL
#define cairo_image_surface_get_stride(cr) 1

struct cairo_surface {
};
typedef struct cairo_surface cairo_surface_t;
#define cairo_image_surface_get_width(surface) 0
#define cairo_image_surface_get_height(surface) 0
#define cairo_image_surface_create(surface, w, h) NULL
#define cairo_surface_destroy(surface)


typedef float co_aix;

typedef struct _area area_t;
struct _area {
    co_aix x, y;
    co_aix w, h;
};
#define range_overlay(as, aw, bs, bw)			\
    (((bs) - (as)) <= (aw) || ((as) - (bs)) <= (bw))
#define areas_are_overlay(a1, a2)		\
    (range_overlay((a1)->x, (a1)->w,		\
		   (a2)->x, (a2)->w) &&		\
     range_overlay((a1)->y, (a1)->h,		\
		   (a2)->y, (a2)->h))

struct mb_obj {
    int obj_type;
};
typedef struct mb_obj mb_obj_t;

#define GEF_OV_DRAW 0x1
#define GEF_HIDDEN 0x2

struct shape {
    mb_obj_t obj;
    
    area_t area;

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

typedef struct coord coord_t;
struct coord {
    mb_obj_t obj;
  
    area_t area;
    int flags;
    coord_t *parent;
    coord_t *children;
    coord_t *sibling;
    shape_t *shapes;
};

#define COF_SKIP 0x1

#define coord_get_area(coord) (&(coord)->area)
#define FOR_COORD_SHAPES(coord, shape)		\
    for(shape = (coord)->shapes;		\
	shape != NULL;				\
	shape = (shape)->sibling)
#define FOR_COORDS_PREORDER(root, last)			\
    for(last = root;					\
	last != NULL;					\
	last = preorder_coord_subtree(root, last))

static
coord_t *preorder_coord_subtree(coord_t *root, coord_t *last) {
    if(last->children)
	return last->children;
    while(last->sibling == NULL)
	last = last->parent;
    return last->sibling;
}

static
coord_t *postorder_coord_subtree(coord_t *root, coord_t *last) {
    coord_t *cur;

    if(last != NULL) {
	if(last->sibling == NULL) {
	    cur = last->parent;
	    return cur;
	}
	cur = last->sibling;
    }

    cur = last;
    while(cur->children) {
	cur = cur->children;
    }
    return cur;
}

#define sh_path_draw(path, cr)
#define sh_text_draw(path, cr)
#define sh_rect_draw(path, cr)


struct redraw_man {
    cairo_t *cr;
    int shape_gl_sz;
    shape_t *shape_gl[32];
};
typedef struct redraw_man redraw_man_t;
#define rdman_get_cr(rdman) ((rdman)->cr)
#define rdman_get_gen_geos(rdman) (&(rdman)->gen_geos)
#define rdman_force_clean(rdman) OK
#define rdman_geos(rdman, geo) NULL
#define rdman_clear_shape_gl(rdman)		\
    do {(rdman)->shape_gl_sz = 0; } while(0)
static int rdman_add_shape_gl(redraw_man_t *rdman, shape_t *shape) {
    (rdman)->shape_gl[(rdman)->shape_gl_sz++] = shape;
    return OK;
}
#define rdman_get_shape_gl(rdman, idx)		\
    (rdman)->shape_gl[idx]
#define rdman_shape_gl_len(rdman) (rdman)->shape_gl_sz
static shape_t *rdman_shapes(redraw_man_t *rdman, shape_t *last_shape);


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
    int r;

    if(IS_MBO_SHAPES(obj)) {
	shape = (shape_t *)obj;
	r = _shape_pos_is_in(shape, x, y, in_stroke, rdman_get_cr(rdman));
	return r;
    }
    root = (coord_t *)obj;
    for(cur_coord = postorder_coord_subtree(root, NULL);
	cur_coord != NULL;
	cur_coord = postorder_coord_subtree(root, cur_coord)) {
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
    cairo_surface_t *surface;

    rdman = (redraw_man_t *)malloc(sizeof(redraw_man_t));
    surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 100, 100);
    rdman->cr = cairo_create(surface);
    DARRAY_INIT(&rdman->gen_geos);
    return rdman;
}

static
void _free_fake_rdman(redraw_man_t *rdman) {
    cairo_destroy(rdman->cr);
    DARRAY_DESTROY(&rdman->gen_geos);
    free(rdman);
}

static
void test_mb_obj_pos_is_in(void) {
    redraw_man_t *rdman;
    mb_obj_t *obj;

    rdman = _fake_rdman();
    CU_ASSERT(rdman != NULL);

    _free_fake_rdman(rdman);
}

static
void test_is_obj_objs_overlay(void) {
}

static
void test_mb_objs_are_overlay(void) {
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
