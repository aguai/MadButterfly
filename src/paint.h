#ifndef __PAINT_H_
#define __PAINT_H_

#include <cairo.h>
#include "mb_types.h"
#include "redraw_man.h"

typedef float co_comp_t;

extern paint_t *paint_color_new(redraw_man_t *rdman,
				co_comp_t r, co_comp_t g, co_comp_t b);
extern void paint_color_set(paint_t *paint,
			    co_comp_t r, co_comp_t g, co_comp_t b);

#endif /* __PAINT_H_ */
