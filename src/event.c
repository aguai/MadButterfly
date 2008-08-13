#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include "mb_types.h"
#include "redraw_man.h"
#include "shapes.h"

#define OK 0
#define ERR -1


static int extend_memblk(void **buf, int o_size, int n_size) {
    void *new_buf;

    new_buf = realloc(*buf, n_size);
    if(new_buf == NULL)
	return ERR;

    *buf = new_buf;

    return OK;
}

/*! \brief Add a geo_t object to general geo list.
 *
 * General geo list can use to temporary keep a list of geo_t
 * objects for any purpose.  It supposed to be reused by
 * different modules that need to select part of geo_t objects
 * from a redraw manager.
 */
static int add_gen_geo(redraw_man_t *rdman, geo_t *geo) {
    int max_gen_geos;
    int r;

    if(rdman->n_gen_geos >= rdman->max_gen_geos) {
	max_gen_geos = rdman->n_geos;
	r = extend_memblk((void **)&rdman->gen_geos,
			  sizeof(geo_t *) * rdman->n_gen_geos,
			  sizeof(geo_t *) * max_gen_geos);
	if(r != OK)
	    return ERR;
	rdman->max_gen_geos = max_gen_geos;
    }

    rdman->gen_geos[rdman->n_gen_geos++] = geo;
    return OK;
}

static int collect_shapes_at_point(redraw_man_t *rdman,
				   co_aix x, co_aix y) {
    geo_t *geo;
    int r;
    
    r = rdman_force_clean(rdman);
    if(r != OK)
	return ERR;

    rdman->n_gen_geos = 0;

    for(geo = STAILQ_HEAD(rdman->all_geos);
	geo != NULL;
	geo = STAILQ_NEXT(geo_t, next, geo)) {
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

    geos = rdman->gen_geos;
    cr = rdman->cr;
    for(i = rdman->n_gen_geos - 1; i >= 0; i--) {
	geo = geos[i];
	if(geo->flags & GEF_DIRTY)
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
