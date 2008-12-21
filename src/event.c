/*! \file
 * \brief Convenience functions for event relative work.
 */
#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include "mb_types.h"
#include "mb_redraw_man.h"
#include "mb_shapes.h"

#define OK 0
#define ERR -1
#define FALSE 0
#define TRUE 1

#define ARRAY_EXT_SZ 64


DARRAY_DEFINE(geos, geo_t *);

/*! \brief Add a geo_t object to general geo list.
 *
 * General geo list can use to temporary keep a list of geo_t
 * objects for any purpose.  It supposed to be reused by
 * different modules that need to select part of geo_t objects
 * from a redraw manager.
 */
static int _add_gen_geo(redraw_man_t *rdman, geo_t *geo) {
    int r;

    r = geos_add(&rdman->gen_geos, geo);
    return r == 0? OK: ERR;
}

static int _collect_geos_at_point(redraw_man_t *rdman,
				   co_aix x, co_aix y) {
    geo_t *geo;
    int r;
    
    r = rdman_force_clean(rdman);
    if(r != OK)
	return ERR;

    rdman->gen_geos.num = 0;

    for(geo = rdman_geos(rdman, NULL);
	geo != NULL;
	geo = rdman_geos(rdman, geo)) {
	if(geo_pos_is_in(geo, x, y)) {
	    r = _add_gen_geo(rdman, geo);
	    if(r != OK)
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

static geo_t *find_geo_in_pos(redraw_man_t *rdman,
			      co_aix x, co_aix y, int *in_stroke) {
    geo_t *geo;
    geo_t **geos;
    shape_t *shape;
    cairo_t *cr;
    int i, r;

    geos = rdman->gen_geos.ds;
    cr = rdman->cr;
    for(i = rdman->gen_geos.num - 1; i >= 0; i--) {
	geo = geos[i];
	if(geo->flags & GEF_HIDDEN)
	    continue;
	shape = geo_get_shape(geo);
	r = _shape_pos_is_in_cairo(shape, x, y, in_stroke, cr);
	cairo_new_path(cr);
	if(r)
	    return geo;
    }

    return NULL;
}

shape_t *find_shape_at_pos(redraw_man_t *rdman,
			   co_aix x, co_aix y, int *in_stroke) {
    geo_t *geo;
    int r;

    r = _collect_geos_at_point(rdman, x, y);
    if(r != OK)
	return NULL;

    geo = find_geo_in_pos(rdman, x, y, in_stroke);
    if(geo == NULL)
	return NULL;

    return geo_get_shape(geo);
}

static
int _shape_pos_is_in(redraw_man_t *rdman, shape_t *shape,
		     co_aix x, co_aix y, int *in_stroke) {
    geo_t *geo;
    int r;

    geo = sh_get_geo(shape);
    r = geo_pos_is_in(geo, x, y);
    if(!r)
	return FALSE;

    r = _shape_pos_is_in_cairo(shape, x, y, in_stroke, rdman->cr);
    if(!r)
	return FALSE;

    return TRUE;
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
    geo_t *geo;
    int r;

    if(IS_MBO_SHAPES(obj)) {
	shape = (shape_t *)obj;
	r = _shape_pos_is_in_cairo(shape, x, y, in_stroke, rdman->cr);
	return r;
    }
    root = (coord_t *)obj;
    for(cur_coord = postorder_coord_subtree(root, NULL);
	cur_coord != NULL;
	cur_coord = postorder_coord_subtree(root, cur_coord)) {
	FOR_COORD_MEMBERS(cur_coord, geo) {
	    shape = geo_get_shape(geo);
	    r = _shape_pos_is_in_cairo(shape, x, y, in_stroke, rdman->cr);
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

    rdman_surface = cairo_get_target(rdman->cr);
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

static _draw_to_mask(shape_t *shape, cairo_t *cr) {
    geo_t *geo;

    geo = sh_get_geo(shape);
    if(geo->flags & GEF_OV_DRAW)
	return;
    
    draw_shape_path(shape, cr);
    cairo_clip(cr);
    
    geo->flags |= GEF_OV_DRAW;
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

/*! \brief Is a mb_obj_t overlaid with another mb_object_t and
 *	descendants.
 *
 * coord is relative less than shapes.  Check areas of coord can
 * havily avoid useless computation.  For shapes, it not only check
 * overlay of area.  It also check overlay by actually drawing on a
 * cairo surface.
 */
static
int _is_obj_objs_overlay(mb_obj_t *obj, mb_obj_t *others_root,
			 cairo_t *cr) {
    area_t *area, *candi_area;
    coord_t *coord, *candi_coord, *root;
    shape_t *shape, *candi_shape;
    geo_t *geo, *candi_geo;
    int obj_is_shape;
    int r;
    /*
     */
    
    obj_is_shape = IS_MBO_SHAPES(obj);
    
    if(obj_is_shape) {
	shape = (shape_t *)obj;
	geo = sh_get_geo(shape);
	area = geo_get_area(geo);
    } else {
	coord = (coord_t *)obj;
	area = coord_get_area(coord);
    }
	
    if(IS_MBO_SHAPES(others_root)) {
	candi_shape = (shape_t *)others_root;
	candi_geo = sh_get_geo(candi_shape);
	candi_area =  geo_get_area(candi_geo);
	
	r = is_overlay(area, candi_area);
	if(!r)
	    return FALSE;
	
	if(!obj_is_shape)
	    return TRUE;
	
	_draw_to_mask(candi_shape, cr);
	r = _fill_and_check(shape, cr);
	
	return r;
    }
    
    root = (coord_t *)others_root;
    FOR_COORDS_PREORDER(root, candi_coord) {
	candi_area = coord_get_area(candi_coord);
	r = is_overlay(area, candi_area);
	if(!r) {
	    preorder_coord_skip_subtree(coord);
	    continue;
	}
	
	FOR_COORD_MEMBERS(coord, candi_geo) {
	    candi_area = geo_get_area(candi_geo);
	    r = is_overlay(area, candi_area);
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

/*! \brief Test if two objects are overlaid.
 *
 * \todo Detect overlay in better way with cairo.
 * \note This function cost heavy on CPU power.
 */
int mb_objs_is_overlay(redraw_man_t *rdman,
		       mb_obj_t *obj1, mb_obj_t *obj2) {
    cairo_t *cr;
    area_t *area;
    shape_t *shape;
    geo_t *geo;
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

	FOR_COORD_MEMBERS(coord, geo) {
	    shape = geo_get_shape(geo);
	    r = _is_obj_objs_overlay((mb_obj_t *)shape, obj2, cr);
	    if(r)
		goto out;
	}
    }
    r = FALSE;
    
 out:
    _release_cairo_for_testing(cr);
    return r;
}
