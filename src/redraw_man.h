#ifndef __REDRAW_MAN_H_
#define __REDRAW_MAN_H_

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
    unsigned int next_geo_order;
    int n_geos;
    STAILQ(geo_t) all_geos;

    unsigned int next_coord_order;
    int n_coords;
    coord_t *root_coord;

    elmpool_t *geo_pool;
    elmpool_t *coord_pool;

    int max_dirty_coords;
    int n_dirty_coords;
    coord_t **dirty_coords;

    int max_redrawing_geos;
    int n_redrawing_geos;
    geo_t **redrawing_geos;

    int max_dirty_areas;
    int n_dirty_areas;
    area_t **dirty_areas;
} redraw_man_t;

extern int redraw_man_init(redraw_man_t *rdman);
extern void redraw_man_destroy(redraw_man_t *rdman);
extern int rdman_find_overlaid_shapes(redraw_man_t *rdman,
				      geo_t *geo,
				      geo_t ***overlays);
extern int rdman_add_shape(redraw_man_t *rdman,
			   shape_t *shape, coord_t *coord);
extern int rdman_remove_shape(redraw_man_t *rdman, shape_t *shape);
extern coord_t *rdman_coord_new(redraw_man_t *rdman, coord_t *parent);
extern int rdman_coord_free(redraw_man_t *rdman, coord_t *coord);


#endif /* __REDRAW_MAN_H_ */
