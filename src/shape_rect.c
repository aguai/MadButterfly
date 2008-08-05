#include <stdio.h>
#include "mb_types.h"
#include "shapes.h"

typedef struct _sh_rect {
    shape_t shape;
    co_aix x, y;
    co_aix rx, ry;
    co_aix d_x, d_y;
    co_aix d_rx, d_ry;
} sh_rect_t;

extern void sh_rect_transform(shape_t *shape) {
    sh_rect_t *rect = (sh_rect_t *)shape;

}
