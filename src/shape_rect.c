#include <stdio.h>
#include "mb_types.h"
#include "shapes.h"

typedef struct _sh_rect {
    shape_t shape;
    co_aix x, y;
    co_aix rx, ry;
} sh_rect_t;

