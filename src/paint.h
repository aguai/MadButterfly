#ifndef __PAINT_H_
#define __PAINT_H_

#include <cairo.h>
#include "mb_types.h"
#include "redraw_man.h"
#include "tools.h"

typedef float co_comp_t;

extern paint_t *paint_color_new(redraw_man_t *rdman,
				co_comp_t r, co_comp_t g,
				co_comp_t b, co_comp_t a);
extern void paint_color_set(paint_t *paint,
			    co_comp_t r, co_comp_t g,
			    co_comp_t b, co_comp_t a);
extern void paint_color_get(paint_t *paint,
			    co_comp_t *r, co_comp_t *g,
			    co_comp_t *b, co_comp_t *a);
#define paint_init(_paint, _prepare, _free)	\
     do {					\
	 (_paint)->prepare = _prepare;		\
	 (_paint)->free = _free;		\
	 STAILQ_INIT((_paint)->members);	\
     } while(0)					\


typedef struct _grad_stop {
    co_aix offset;
    co_comp_t r, g, b, a;
} grad_stop_t;

extern paint_t *paint_linear_new(redraw_man_t *rdman,
				 co_aix x1, co_aix y1, co_aix x2, co_aix y2);
extern grad_stop_t *paint_linear_stops(paint_t *paint,
				       int n_stops,
				       grad_stop_t *stops);
extern paint_t *paint_radial_new(redraw_man_t *rdman,
				 co_aix cx, co_aix cy, co_aix r);
extern grad_stop_t *paint_radial_stops(paint_t *paint,
				       int n_stops,
				       grad_stop_t *stops);

#define grad_stop_init(stop, _offset, _r, _g, _b, _a)	\
    do {						\
	(stop)->offset = _offset;			\
	(stop)->r = _r;					\
	(stop)->g = _g;					\
	(stop)->b = _b;					\
	(stop)->a = _a;					\
    } while(0)


#endif /* __PAINT_H_ */
