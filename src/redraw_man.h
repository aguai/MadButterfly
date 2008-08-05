#ifndef __REDRAW_MAN_H_
#define __REDRAW_MAN_H_

#include <cairo.h>
#include "tools.h"
#include "mb_types.h"

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
#ifdef GEO_ORDER
    unsigned int next_geo_order;
#endif
    int n_geos;
    STAILQ(geo_t) all_geos;

    unsigned int next_coord_order;
    int n_coords;
    coord_t *root_coord;

    elmpool_t *geo_pool;
    elmpool_t *coord_pool;
    elmpool_t *shnode_pool;

    int max_dirty_coords;
    int n_dirty_coords;
    coord_t **dirty_coords;

    int max_dirty_geos;
    int n_dirty_geos;
    geo_t **dirty_geos;

    int max_dirty_areas;
    int n_dirty_areas;
    area_t **dirty_areas;

    int max_gen_geos;
    int n_gen_geos;
    geo_t **gen_geos;		/* general geo list */

    cairo_t *cr;
    cairo_t *backend;
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
extern int rdman_coord_changed(redraw_man_t *rdman, coord_t *coord);
extern int rdman_shape_changed(redraw_man_t *rdman, shape_t *shape);
extern int rdman_redraw_changed(redraw_man_t *rdman);
extern int rdman_redraw_all(redraw_man_t *rdman);
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


#endif /* __REDRAW_MAN_H_ */
