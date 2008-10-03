#ifndef __REDRAW_MAN_H_
#define __REDRAW_MAN_H_

#include <cairo.h>
#include "tools.h"
#include "mb_types.h"
#include "observer.h"

DARRAY(coords, coord_t *);
DARRAY(geos, geo_t *);
DARRAY(areas, area_t *);

/*! \brief Manage redrawing of shapes (graphic elements).
 *
 * Every coord_t and geo_t object is assigned with a unique
 * incremental order.  The order is a unsigned integer.
 * Every time a new coord_t or geo_t object is added, it is
 * assigned with a order number that 1 bigger than last one
 * until reaching maximum of unsigned integer.
 * When a maximum is meet, all coord_t or geo_t objects
 * are reasigned with a new order number from 1.  It means
 * order numbers that have been assigned and then removed
 * later are recycled.
 *
 * Dirty flag is clear when the transformation matrix of a coord
 * object been recomputed or when a geo_t objects been redrawed.
 */
typedef struct _redraw_man {
    unsigned int next_coord_order;
    int n_coords;
    coord_t *root_coord;

    elmpool_t *geo_pool;
    elmpool_t *coord_pool;
    elmpool_t *shnode_pool;
    elmpool_t *observer_pool;
    elmpool_t *subject_pool;
    elmpool_t *paint_color_pool;

    coords_t dirty_coords;
    geos_t dirty_geos;
    areas_t dirty_areas;

    geos_t gen_geos;

    coords_t free_coords;
    geos_t free_geos;

    cairo_t *cr;
    cairo_t *backend;

    ob_factory_t ob_factory;

    subject_t *redraw;		/*!< \brief Notified after redrawing. */
} redraw_man_t;

extern int redraw_man_init(redraw_man_t *rdman, cairo_t *cr,
			   cairo_t *backend);
extern void redraw_man_destroy(redraw_man_t *rdman);
extern int rdman_find_overlaid_shapes(redraw_man_t *rdman,
				      geo_t *geo,
				      geo_t ***overlays);
extern int rdman_add_shape(redraw_man_t *rdman,
			   shape_t *shape, coord_t *coord);
extern int rdman_remove_shape(redraw_man_t *rdman, shape_t *shape);
extern coord_t *rdman_coord_new(redraw_man_t *rdman, coord_t *parent);
extern int rdman_coord_free(redraw_man_t *rdman, coord_t *coord);
extern int rdman_coord_subtree_free(redraw_man_t *rdman, coord_t *subtree);
extern int rdman_coord_changed(redraw_man_t *rdman, coord_t *coord);
extern int rdman_shape_changed(redraw_man_t *rdman, shape_t *shape);
extern int rdman_redraw_changed(redraw_man_t *rdman);
extern int rdman_redraw_all(redraw_man_t *rdman);
extern int rdman_redraw_area(redraw_man_t *rdman, co_aix x, co_aix y,
			     co_aix w, co_aix h);
extern geo_t *rdman_geos(redraw_man_t *rdman, geo_t *last);
extern int rdman_force_clean(redraw_man_t *rdman);
extern shnode_t *shnode_new(redraw_man_t *rdman, shape_t *shape);
#define shnode_free(rdman, node) elmpool_elm_free((rdman)->shnode_pool, node)
#define shnode_list_free(rdman, q)				\
    do {							\
	shnode_t *__node, *__last;				\
	__last = STAILQ_HEAD(q);				\
	if(__last == NULL) break;				\
	for(__node = STAILQ_NEXT(shnode_t, next, __last);	\
	    __node != NULL;					\
	    __node = STAILQ_NEXT(shnode_t, next, __node)) {	\
	    shnode_free(rdman, __last);				\
	    __last = __node;					\
	}							\
	shnode_free(rdman, __last);				\
    } while(0)
#define _rdman_paint_child(rdman, paint, shape)		\
    do {						\
	shnode_t *__node;				\
	if((shape)->fill != (paint) &&			\
	   (shape)->stroke != (paint)) {		\
	    __node = shnode_new(rdman, shape);		\
	    STAILQ_INS_TAIL((paint)->members,		\
			    shnode_t, next, __node);	\
	}						\
    } while(0)
#define rdman_paint_fill(rdman, paint, shape)		\
    do {						\
	_rdman_paint_child(rdman, paint, shape);	\
	shape->fill = paint;				\
    } while(0)
#define rdman_paint_stroke(rdman, paint, shape)		\
    do {						\
	_rdman_paint_child(rdman, paint, shape);	\
	shape->stroke = paint;				\
    } while(0)
extern int rdman_paint_changed(redraw_man_t *rdman, paint_t *paint);

extern shape_t *find_shape_at_pos(redraw_man_t *rdman,
				  co_aix x, co_aix y, int *in_stroke);
#define rdman_get_ob_factory(rdman) (&(rdman)->ob_factory)
#define rdman_get_redraw_subject(rdman) ((rdman)->redraw)


#endif /* __REDRAW_MAN_H_ */
