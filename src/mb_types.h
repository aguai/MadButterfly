#ifndef __MB_TYPES_H_
#define __MB_TYPES_H_

typedef float co_aix;
typedef struct _shape shape_t;

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
    shape_t *members;
} coord_t;


typedef struct _geo geo_t;
struct _shape {
    int sh_type;
    geo_t *geo;
    struct _shape *sibling;
};

enum { SHT_UNKNOW, SHT_PATH, SHT_TEXT };

extern void coord_init(coord_t *co, coord_t *parent);
extern void coord_trans_pos(coord_t *co, co_aix *x, co_aix *y);
extern void update_aggr_matrix(coord_t *start);

#endif /* __MB_TYPES_H_ */
