#ifndef __MB_TYPES_H_
#define __MB_TYPES_H_

typedef float co_aix;

/*! \brief A coordination system.
 *
 * It have a transform function defined by matrix to transform
 * coordination from source space to target space.
 * Source space is where the contained is drawed, and target space
 * is where the coordination of parent container of the element
 * represented by this coord object.
 */
typedef struct _coord {
    int seq;
    co_aix matrix[6];
    co_aix aggr_matrix[6];
    struct _coord *parent;
    struct _coord *children, *sibling;
} coord_t;


typedef struct _geo geo_t;
typedef struct _shape {
    int sh_type;
    geo_t *geo;
} shape_t;

enum { SHT_UNKNOW, SHT_PATH, SHT_TEXT };

extern void coord_init(coord_t *co, coord_t *parent);
extern void coord_trans_pos(coord_t *co, co_aix *x, co_aix *y);

#endif /* __MB_TYPES_H_ */
