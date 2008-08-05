#ifndef __MB_TYPES_H_
#define __MB_TYPES_H_

#include <cairo.h>
#include "tools.h"

typedef float co_aix;
typedef struct _shape shape_t;
typedef struct _geo geo_t;
typedef struct _area area_t;
typedef struct _shnode shnode_t;
typedef struct _paint paint_t;

struct _paint {
    void (*prepare)(paint_t *paint, cairo_t *cr);
    void (*free)(paint_t *paint);
    STAILQ(shnode_t) members;
};

struct _shnode {
    shape_t *shape;
    shnode_t *next;
};

struct _area {
    co_aix x, y;
    co_aix w, h;
};

/*! \brief Geometry data of a shape or a group of shape.
 */
struct _geo {
#ifdef GEO_ORDER
    unsigned int order;
#endif
    unsigned int flags;
    shape_t *shape;
    geo_t *next;		/*!< \brief Link all geo objects. */

    area_t *cur_area, *last_area;
    area_t areas[2];
};
#define GEF_DIRTY 0x1

extern int is_overlay(area_t *r1, area_t *r2);
extern void area_init(area_t *area, int n_pos, co_aix pos[][2]);
extern void geo_init(geo_t *g);
extern void geo_from_positions(geo_t *g, int n_pos, co_aix pos[][2]);
extern void geo_mark_overlay(geo_t *g, int n_others, geo_t **others,
			     int *n_overlays, geo_t **overlays);
#define geo_get_shape(g) ((g)->shape)
#define geo_set_shape(g, sh) do {(g)->shape = sh;} while(0)
#define _geo_is_in(a, s, w) ((a) >= (s) && (a) < ((s) + (w)))
#define geo_pos_is_in(g, _x, _y)				\
    (_geo_is_in(_x, (g)->cur_area->x, (g)->cur_area->w) &&	\
     _geo_is_in(_y, (g)->cur_area->y, (g)->cur_area->h))


/*! \brief A coordination system.
 *
 * It have a transform function defined by matrix to transform
 * coordination from source space to target space.
 * Source space is where the contained is drawed, and target space
 * is where the coordination of parent container of the element
 * represented by this coord object.
 *
 * \dot
 * digraph G {
 * graph [rankdir=LR];
 * root -> child00 -> child10 -> child20 [label="children" color="blue"];
 * child00 -> child01 -> child02 [label="sibling"];
 * child10 -> child11 [label="sibling"];
 * }
 * \enddot
 */
typedef struct _coord {
    unsigned int order;
    unsigned int flags;
    area_t *cur_area, *last_area;
    area_t areas[2];

    co_aix matrix[6];
    co_aix aggr_matrix[6];

    struct _coord *parent;
    STAILQ(struct _coord) children;
    struct _coord *sibling;

    STAILQ(shape_t) members;	/*!< All shape_t objects in this coord. */
} coord_t;
#define COF_DIRTY 0x1

extern void coord_init(coord_t *co, coord_t *parent);
extern void coord_trans_pos(coord_t *co, co_aix *x, co_aix *y);
extern void compute_aggr_of_coord(coord_t *coord);
extern void update_aggr_matrix(coord_t *start);
extern coord_t *preorder_coord_subtree(coord_t *root, coord_t *last);


/*! \brief A grahpic shape.
 *
 * \dot
 * digraph G {
 * "shape" -> "coord";
 * "shape" -> "geo";
 * "geo" -> "shape";
 * "coord" -> "shape" [label="members"]
 * "shape" -> "shape" [label="sibling"];
 * }
 * \enddot
 */
struct _shape {
    int sh_type;
    geo_t *geo;
    coord_t *coord;
    shape_t *coord_mem_next;
    paint_t *fill, *stroke;
    co_aix stroke_width;
    int stroke_linecap;
    int stroke_linejoin;
};
enum { SHT_UNKNOW, SHT_PATH, SHT_TEXT };

#define sh_attach_geo(sh, g)			\
    do {					\
	(sh)->geo = g;				\
	(g)->shape = (shape_t *)(sh);		\
    } while(0)
#define sh_detach_geo(sh)			\
    do {					\
	(sh)->geo->shape = NULL;		\
	(sh)->geo = NULL;			\
    } while(0)
extern void sh_attach_coord(shape_t *sh, coord_t *coord);
extern void sh_detach_coord(shape_t *sh);

#endif /* __MB_TYPES_H_ */
