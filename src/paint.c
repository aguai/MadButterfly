#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include "mb_paint.h"

#define ASSERT(x)

/*! \brief Solid color paint.
 */
typedef struct _paint_color {
    paint_t paint;
    co_comp_t r, g, b, a;
} paint_color_t;

int _paint_color_size = sizeof(paint_color_t);


static void paint_color_prepare(paint_t *paint, cairo_t *cr) {
    paint_color_t *color = (paint_color_t *)paint;

    cairo_set_source_rgba(cr, color->r, color->g, color->b, color->a);
}

static void paint_color_free(redraw_man_t *rdman, paint_t *paint) {
    shnode_list_free(rdman, paint->members);
    paint_destroy(paint);
    elmpool_elm_free(rdman->paint_color_pool, paint);
}

paint_t *rdman_paint_color_new(redraw_man_t *rdman,
			       co_comp_t r, co_comp_t g,
			       co_comp_t b, co_comp_t a) {
    paint_color_t *color;

    color = (paint_color_t *)elmpool_elm_alloc(rdman->paint_color_pool);
    if(color == NULL)
	return NULL;
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = a;
    paint_init(&color->paint, paint_color_prepare, paint_color_free);
    return (paint_t *)color;
}

void paint_color_set(paint_t *paint,
		     co_comp_t r, co_comp_t g,
		     co_comp_t b, co_comp_t a) {
    paint_color_t *color = (paint_color_t *)paint;

    color->r = r;
    color->g = g;
    color->b = b;
    color->a = a;
}

void paint_color_get(paint_t *paint,
		     co_comp_t *r, co_comp_t *g,
		     co_comp_t *b, co_comp_t *a) {
    paint_color_t *color = (paint_color_t *)paint;

    *r = color->r;
    *g = color->g;
    *b = color->b;
    *a = color->a;
}

/*! \brief Linear gradient.
 */
typedef struct _paint_linear {
    paint_t paint;
    co_aix x1, y1;
    co_aix x2, y2;
    int n_stops;
    grad_stop_t *stops;
    int flags;
    cairo_pattern_t *ptn;
} paint_linear_t;

#define LIF_DIRTY 0x1

static void paint_linear_prepare(paint_t *paint, cairo_t *cr) {
    paint_linear_t *linear = (paint_linear_t *)paint;
    cairo_pattern_t *ptn;
    grad_stop_t *stop;
    int i;

    ptn = linear->ptn;
    if(linear->flags & LIF_DIRTY) {
	if(ptn)
	    cairo_pattern_destroy(ptn);
	linear->flags &= ~LIF_DIRTY;
	ptn = cairo_pattern_create_linear(linear->x1, linear->y1,
					  linear->x2, linear->y2);
	for(i = 0; i < linear->n_stops; i++) {
	    stop = &linear->stops[i];
	    cairo_pattern_add_color_stop_rgba(ptn, stop->offset,
					      stop->r, stop->g, stop->b,
					      stop->a);
	}
	linear->ptn = ptn;
    }

    cairo_set_source(cr, ptn);
}

static void paint_linear_free(redraw_man_t *rdman, paint_t *paint) {
    paint_linear_t *linear = (paint_linear_t *)paint;

    if(linear->ptn)
	cairo_pattern_destroy(linear->ptn);
    paint_destroy(paint);
    free(paint);
}

paint_t *rdman_paint_linear_new(redraw_man_t *rdman,
				co_aix x1, co_aix y1,
				co_aix x2, co_aix y2) {
    paint_linear_t *linear;

    linear = (paint_linear_t *)malloc(sizeof(paint_linear_t));
    if(linear == NULL)
	return NULL;

    paint_init(&linear->paint, paint_linear_prepare, paint_linear_free);

    linear->x1 = x1;
    linear->y1 = y1;
    linear->x2 = x2;
    linear->y2 = y2;
    linear->n_stops = 0;
    linear->stops = NULL;
    linear->flags = LIF_DIRTY;
    linear->ptn = NULL;

    return (paint_t *)linear;
}

/*! \brief Setup color stop for a linear radient paint.
 *
 * stops should be managed by users of the function.  It should be
 * available before the paint being freed or changed to another
 * array of stops.
 */
grad_stop_t *paint_linear_stops(paint_t *paint,
				int n_stops,
				grad_stop_t *stops) {
    paint_linear_t *linear = (paint_linear_t *)paint;
    grad_stop_t *old_stops;
    
    old_stops = linear->stops;
    linear->n_stops = n_stops;
    linear->stops = stops;
    linear->flags |= LIF_DIRTY;

    return old_stops;
}

/*! \brief Radial gradient.
 *
 * NOTE: The only supported gradient unit is userSpaceOnUse.
 */
typedef struct _paint_radial {
    paint_t paint;
    co_aix cx, cy;
    co_aix r;
    int n_stops;
    grad_stop_t *stops;
    int flags;
    cairo_pattern_t *ptn;
} paint_radial_t;

#define RDF_DIRTY 0x1

static void paint_radial_prepare(paint_t *paint, cairo_t *cr) {
    paint_radial_t *radial = (paint_radial_t *)paint;
    cairo_pattern_t *ptn;
    grad_stop_t *stop;
    int i;

    if(radial->flags & RDF_DIRTY) {
	ptn = cairo_pattern_create_radial(radial->cx, radial->cy, 0,
					  radial->cx, radial->cy,
					  radial->r);
	ASSERT(ptn != NULL);
	stop = radial->stops;
	for(i = 0; i < radial->n_stops; i++, stop++) {
	    cairo_pattern_add_color_stop_rgba(ptn, stop->offset,
					      stop->r, stop->g,
					      stop->b, stop->a);
	}
	cairo_pattern_destroy(radial->ptn);
	radial->ptn = ptn;
    }
    cairo_set_source(cr, radial->ptn);
}

static void paint_radial_free(redraw_man_t *rdman, paint_t *paint) {
    paint_radial_t *radial = (paint_radial_t *)paint;

    if(radial->ptn)
	cairo_pattern_destroy(radial->ptn);
    paint_destroy(paint);
    free(paint);
}

paint_t *rdman_paint_radial_new(redraw_man_t *rdman,
				co_aix cx, co_aix cy, co_aix r) {
    paint_radial_t *radial;

    radial = O_ALLOC(paint_radial_t);
    if(radial == NULL)
	return NULL;

    paint_init(&radial->paint, paint_radial_prepare, paint_radial_free);
    radial->cx = cx;
    radial->cy = cy;
    radial->r = r;
    radial->n_stops = 0;
    radial->stops = NULL;
    radial->flags = RDF_DIRTY;
    radial->ptn = NULL;

    return (paint_t *)radial;
}

/*! \brief Setup color stop for a radial radient paint.
 *
 * stops should be managed by users of the function.  It should be
 * available before the paint being freed or changed to another
 * array of stops.
 */
grad_stop_t *paint_radial_stops(paint_t *paint,
				int n_stops,
				grad_stop_t *stops) {
    paint_radial_t *radial = (paint_radial_t *)paint;
    grad_stop_t *old_stops;
    
    old_stops = radial->stops;
    radial->n_stops = n_stops;
    radial->stops = stops;
    radial->flags |= RDF_DIRTY;

    return old_stops;
}


typedef struct _paint_image {
    paint_t paint;
    mb_img_data_t *img;
    cairo_surface_t *surf;
    cairo_pattern_t *ptn;
} paint_image_t;

static
void paint_image_prepare(paint_t *paint, cairo_t *cr) {
    paint_image_t *paint_img = (paint_image_t *)paint;
    mb_img_data_t *img_data;

    img_data = paint_img->img;
    cairo_set_source(cr, paint_img->ptn);
}

static
void paint_image_free(redraw_man_t *rdman, paint_t *paint) {
    paint_image_t *paint_img = (paint_image_t *)paint;
    mb_img_data_t *img_data;
    
    cairo_surface_destroy(paint_img->surf);
    img_data = paint_img->img;
    MB_IMG_DATA_FREE(img_data);
    paint_destroy(&paint_img->paint);
    free(paint);
}

paint_t *rdman_paint_image_new(redraw_man_t *rdman,
			       mb_img_data_t *img) {
    paint_image_t *paint;
    int fmt;

    switch(img->fmt) {
    case MB_IFMT_ARGB32:
	fmt = CAIRO_FORMAT_ARGB32;
	break;
    case MB_IFMT_RGB24:
	fmt = CAIRO_FORMAT_RGB24;
	break;
    case MB_IFMT_A8:
	fmt = CAIRO_FORMAT_A8;
	break;
    case MB_IFMT_A1:
	fmt = CAIRO_FORMAT_A1;
	break;
    default:
	return NULL;
    }
    
    paint = O_ALLOC(paint_image_t);
    if(paint == NULL)
	return NULL;
    
    paint_init(&paint->paint, paint_image_prepare, paint_image_free);
    paint->img = img;
    paint->surf = cairo_image_surface_create_for_data(img->content,
						      fmt,
						      img->width,
						      img->height,
						      img->stride);
    if(paint->surf == NULL) {
	paint_destroy(&paint->paint);
	free(paint);
	return NULL;
    }
    
    paint->ptn = cairo_pattern_create_for_surface(paint->surf);
    if(paint->ptn == NULL) {
	paint_destroy(&paint->paint);
	cairo_surface_destroy(paint->surf);
	free(paint);
	return NULL;
    }

    return (paint_t *)paint;
}

void paint_image_set_matrix(paint_t *paint, co_aix matrix[6]) {
    paint_image_t *img_paint = (paint_image_t *)paint;
    cairo_matrix_t cmatrix;
    
    cmatrix.xx = matrix[0];
    cmatrix.xy = matrix[1];
    cmatrix.x0 = matrix[2];
    cmatrix.yx = matrix[3];
    cmatrix.yy = matrix[4];
    cmatrix.y0 = matrix[5];
    cairo_pattern_set_matrix(img_paint->ptn, &cmatrix);
}
