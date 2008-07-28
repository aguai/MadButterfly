#ifndef __MB_TYPES_H_
#define __MB_TYPES_H_

#include "tools.h"

typedef float co_aix;
typedef struct _shape shape_t;
typedef struct _geo geo_t;

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
    unsigned int seq;
    co_aix matrix[6];
    co_aix aggr_matrix[6];
    struct _coord *parent;
    STAILQ(struct _coord) children;
    struct _coord *sibling;
    STAILQ(shape_t) members;	/*!< All shape objects in this coord. */
} coord_t;

extern void coord_init(coord_t *co, coord_t *parent);
extern void coord_trans_pos(coord_t *co, co_aix *x, co_aix *y);
extern void update_aggr_matrix(coord_t *start);
extern coord_t *preorder_coord_tree(coord_t *last);


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
    struct _shape *sibling;
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


/*! \brief Geometry data of a shape or a group of shape.
 */
struct _geo {
    shape_t *shape;
    co_aix x, y;
    co_aix w, h;
    geo_t *next;		/*!< \brief Link geo_t objects. */
    unsigned int seq;
};

extern int is_overlay(geo_t *r1, geo_t *r2);
extern void geo_init(geo_t *g, int n_pos, co_aix pos[][2]);
extern void geo_mark_overlay(geo_t *g, int n_others, geo_t **others,
			     int *n_overlays, geo_t **overlays);
#define geo_get_shape(g) ((g)->shape)
#define geo_set_shape(g, sh) do {(g)->shape = sh;} while(0)

#endif /* __MB_TYPES_H_ */
