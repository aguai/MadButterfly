#ifndef __SHAPES_H_
#define __SHAPES_H_

#include <cairo.h>
#include "mb_types.h"

extern void sh_path_free(shape_t *path);
extern shape_t *sh_path_new(char *data);
extern void sh_path_transform(shape_t *shape);
extern void sh_path_fill(shape_t *shape, cairo_t *cr);
extern void sh_path_stroke(shape_t *shape, cairo_t *cr);

#endif /* __SHAPES_H_ */
