#ifndef __REDRAW_MAN_H_
#define __REDRAW_MAN_H_

#include "tools.h"
#include "mb_types.h"

/*! \brief Manage redrawing of shapes (graphic elements). */
typedef struct _redraw_man {
    int n_geos;
    STAILQ(geo_t) all_geos;
    coord_t *root_coord;
    elmpool_t *geo_pool;
    elmpool_t *coord_pool;
    unsigned int seq;
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
