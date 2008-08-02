#ifndef __PAINT_H_
#define __PAINT_H_

#include <cairo.h>
#include "mb_types.h"
#include "redraw_man.h"
#include "tools.h"

typedef float co_comp_t;

extern paint_t *paint_color_new(redraw_man_t *rdman,
				co_comp_t r, co_comp_t g, co_comp_t b);
extern void paint_color_set(paint_t *paint,
			    co_comp_t r, co_comp_t g, co_comp_t b);
#define paint_init(_paint, _prepare, _free)	\
     do {					\
	 (_paint)->prepare = _prepare;		\
	 (_paint)->free = _free;		\
	 STAILQ_INIT((_paint)->members);	\
     } while(0)					\

#endif /* __PAINT_H_ */
