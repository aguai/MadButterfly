#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include "mb_types.h"
#include "redraw_man.h"
#include "shapes.h"

#define OK 0
#define ERR -1

#define ARRAY_EXT_SZ 64


static int extend_memblk(void **buf, int o_size, int n_size) {
    void *new_buf;

    new_buf = realloc(*buf, n_size);
    if(new_buf == NULL)
	return ERR;

    *buf = new_buf;

    return OK;
}

DARRAY_DEFINE(geos, geo_t *);

/*! \brief Add a geo_t object to general geo list.
 *
 * General geo list can use to temporary keep a list of geo_t
 * objects for any purpose.  It supposed to be reused by
 * different modules that need to select part of geo_t objects
 * from a redraw manager.
 */
static int add_gen_geo(redraw_man_t *rdman, geo_t *geo) {
    int r;

    r = geos_add(&rdman->gen_geos, geo);
    return r == 0? OK: ERR;
}

static int collect_shapes_at_point(redraw_man_t *rdman,
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
	    r = add_gen_geo(rdman, geo);
	    if(r != OK)
		return ERR;
	}
    }

    return OK;
}

static void draw_shape_path(shape_t *shape, cairo_t *cr) {
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
    }
}

static geo_t *find_geo_in_pos(redraw_man_t *rdman,
			      co_aix x, co_aix y, int *in_stroke) {
    geo_t *geo;
    geo_t **geos;
    shape_t *shape;
    cairo_t *cr;
    int i;

    geos = rdman->gen_geos.ds;
    cr = rdman->cr;
    for(i = rdman->gen_geos.num - 1; i >= 0; i--) {
	geo = geos[i];
	if(geo->flags & GEF_HIDDEN)
	    continue;
	shape = geo->shape;
	draw_shape_path(shape, cr);
	if(shape->fill) {
	    if(cairo_in_fill(cr, x, y)) {
		*in_stroke = 0;
		cairo_new_path(rdman->cr);
		return geo;
	    }
	}
	if(shape->stroke) {
	    if(cairo_in_stroke(cr, x, y)) {
		*in_stroke = 1;
		cairo_new_path(rdman->cr);
		return geo;
	    }
	}
	cairo_new_path(rdman->cr);
    }

    return NULL;
}

shape_t *find_shape_at_pos(redraw_man_t *rdman,
			   co_aix x, co_aix y, int *in_stroke) {
    geo_t *geo;
    int r;

    r = collect_shapes_at_point(rdman, x, y);
    if(r != OK)
	return NULL;

    geo = find_geo_in_pos(rdman, x, y, in_stroke);
    if(geo == NULL)
	return NULL;

    return geo->shape;
}
