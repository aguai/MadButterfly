// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdio.h>
#include <stdlib.h>
#include "mb_graph_engine.h"
#include "mb_paint.h"

#define ASSERT(x)

/*! \brief Solid color paint.
 */
typedef struct _paint_color {
    paint_t paint;
    co_comp_t r, g, b, a;
} paint_color_t;

int _paint_color_size = sizeof(paint_color_t);


static void paint_color_prepare(paint_t *paint, mbe_t *cr, shape_t *sh) {
    paint_color_t *color = (paint_color_t *)paint;

    mbe_set_source_rgba(cr, color->r, color->g, color->b, color->a);
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
    paint_init(&color->paint, MBP_COLOR,
	       paint_color_prepare, paint_color_free);
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
    mbe_pattern_t *ptn;
} paint_linear_t;

#define LIF_DIRTY 0x1

int _paint_linear_size = sizeof(paint_linear_t);

static void paint_linear_prepare(paint_t *paint, mbe_t *cr, shape_t *sh) {
    paint_linear_t *linear = (paint_linear_t *)paint;
    mbe_pattern_t *ptn;
    co_aix x1, y1;
    co_aix x2, y2;
    co_aix *mtx;

    ptn = linear->ptn;
    if(linear->flags & LIF_DIRTY) {
	mtx = sh_get_aggr_matrix(sh);
	x1 = linear->x1;
	y1 = linear->y1;
	matrix_trans_pos(mtx, &x1, &y1);
	x2 = linear->x2;
	y2 = linear->y2;
	matrix_trans_pos(mtx, &x2, &y2);

	if(ptn)
	    mbe_pattern_destroy(ptn);
	linear->flags &= ~LIF_DIRTY;
	ptn = mbe_pattern_create_linear(x1, y1, x2, y2,
					linear->stops, linear->n_stops);
	ASSERT(ptn != NULL);
	linear->ptn = ptn;
    }

    mbe_set_source(cr, ptn);
}

static void paint_linear_free(redraw_man_t *rdman, paint_t *paint) {
    paint_linear_t *linear = (paint_linear_t *)paint;

    if(linear->ptn)
	mbe_pattern_destroy(linear->ptn);
    paint_destroy(paint);
    elmpool_elm_free(rdman->paint_linear_pool, linear);
}

paint_t *rdman_paint_linear_new(redraw_man_t *rdman,
				co_aix x1, co_aix y1,
				co_aix x2, co_aix y2) {
    paint_linear_t *linear;

    linear = (paint_linear_t *)elmpool_elm_alloc(rdman->paint_linear_pool);
    if(linear == NULL)
	return NULL;

    paint_init(&linear->paint, MBP_LINEAR,
	       paint_linear_prepare, paint_linear_free);

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
    mbe_pattern_t *ptn;
} paint_radial_t;

#define RDF_DIRTY 0x1

#define pnt_radial_clear_flags(radial, _flags)	\
    do {					\
	(radial)->flags &= ~(_flags);		\
    } while(0)

int _paint_radial_size = sizeof(paint_radial_t);

static void paint_radial_prepare(paint_t *paint, mbe_t *cr, shape_t *sh) {
    paint_radial_t *radial = (paint_radial_t *)paint;
    mbe_pattern_t *ptn;
    co_aix cx, cy;
    co_aix *mtx;

    if(radial->flags & RDF_DIRTY) {
	mtx = sh_get_aggr_matrix(sh);
	cx = radial->cx;
	cy = radial->cy;
	matrix_trans_pos(mtx, &cx, &cy);
	
	ptn = mbe_pattern_create_radial(cx, cy, 0,
					cx, cy,
					radial->r,
					radial->stops,
					radial->n_stops);
	ASSERT(ptn != NULL);
	if(radial->ptn)
	    mbe_pattern_destroy(radial->ptn);
	radial->ptn = ptn;
	
	pnt_radial_clear_flags(radial, RDF_DIRTY);
    }
    mbe_set_source(cr, radial->ptn);
}

static void paint_radial_free(redraw_man_t *rdman, paint_t *paint) {
    paint_radial_t *radial = (paint_radial_t *)paint;

    if(radial->ptn)
	mbe_pattern_destroy(radial->ptn);
    paint_destroy(paint);
    elmpool_elm_free(rdman->paint_radial_pool, radial);
}

paint_t *rdman_paint_radial_new(redraw_man_t *rdman,
				co_aix cx, co_aix cy, co_aix r) {
    paint_radial_t *radial;

    radial = elmpool_elm_alloc(rdman->paint_radial_pool);
    if(radial == NULL)
	return NULL;

    paint_init(&radial->paint, MBP_RADIAL,
	       paint_radial_prepare, paint_radial_free);
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


/*! \brief Using an image as a paint.
 *
 * This type of paints fill/stroke shapes with an image.
 */
typedef struct _paint_image {
    paint_t paint;
    mb_img_data_t *img;
    mbe_pattern_t *ptn;
} paint_image_t;

int _paint_image_size = sizeof(paint_image_t);

static
void paint_image_prepare(paint_t *paint, mbe_t *cr, shape_t *sh) {
    paint_image_t *paint_img = (paint_image_t *)paint;
    
    mbe_set_source(cr, paint_img->ptn);
}

static
void paint_image_free(redraw_man_t *rdman, paint_t *paint) {
    paint_image_t *paint_img = (paint_image_t *)paint;
    mb_img_data_t *img_data;

    img_data = paint_img->img;
    MB_IMG_DATA_FREE(img_data);
    paint_destroy(&paint_img->paint);
    mbe_pattern_destroy(paint_img->ptn);
    elmpool_elm_free(rdman->paint_image_pool, paint_img);
}

/*! \brief Create an image painter.
 *
 * Create a painter that fill/stroke shapes with an image.
 *
 * \param img is image data return by image load.
 *            Owner-ship of img is transfered.
 */
paint_t *rdman_paint_image_new(redraw_man_t *rdman,
			       mb_img_data_t *img) {
    paint_image_t *paint;

    paint = elmpool_elm_alloc(rdman->paint_image_pool);
    if(paint == NULL)
	return NULL;

    paint_init(&paint->paint, MBP_IMAGE,
	       paint_image_prepare, paint_image_free);
    paint->img = img;
    paint->ptn = mbe_pattern_create_image(img);
    if(paint->ptn == NULL) {
	paint_destroy(&paint->paint);
	elmpool_elm_free(rdman->paint_image_pool, paint);
	return NULL;
    }

    return (paint_t *)paint;
}

/*! \brief Setting transformation from user space to image space.
 *
 * This transformation matrix maps points drawed in user space to
 * corresponding points in image space.  It is used to resample
 * the image to generate pixels of result image.
 */
void paint_image_set_matrix(paint_t *paint, co_aix matrix[6]) {
    paint_image_t *img_paint = (paint_image_t *)paint;

    mbe_pattern_set_matrix(img_paint->ptn, matrix);
}

void paint_image_get_size(paint_t *paint, int *w, int *h) {
    paint_image_t *ipaint = (paint_image_t *)paint;

    ASSERT(paint->pnt_type == MBP_IMAGE);
    *w = ipaint->img->w;
    *h = ipaint->img->h;
}
