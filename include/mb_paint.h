#ifndef __PAINT_H_
#define __PAINT_H_

#include "mb_graph_engine.h"
#include "mb_types.h"
#include "mb_redraw_man.h"
#include "mb_img_ldr.h"
#include "mb_tools.h"

extern paint_t *rdman_paint_color_new(redraw_man_t *rdman,
				      co_comp_t r, co_comp_t g,
				      co_comp_t b, co_comp_t a);
extern void paint_color_set(paint_t *paint,
			    co_comp_t r, co_comp_t g,
			    co_comp_t b, co_comp_t a);
extern void paint_color_get(paint_t *paint,
			    co_comp_t *r, co_comp_t *g,
			    co_comp_t *b, co_comp_t *a);
#define paint_init(_paint, _type, _prepare, _free)	\
    do {						\
	(_paint)->pnt_type = _type;			\
	(_paint)->flags = 0;				\
	(_paint)->prepare = _prepare;			\
	(_paint)->free = _free;				\
	STAILQ_INIT((_paint)->members);			\
	(_paint)->pnt_next = NULL;			\
    } while(0)
#define paint_destroy(_paint)


extern paint_t *rdman_paint_linear_new(redraw_man_t *rdman,
				       co_aix x1, co_aix y1,
				       co_aix x2, co_aix y2);
extern grad_stop_t *paint_linear_stops(paint_t *paint,
				       int n_stops,
				       grad_stop_t *stops);
extern paint_t *rdman_paint_radial_new(redraw_man_t *rdman,
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

/*! \brief A paint that fill or stroke shape with an image.
 */
extern paint_t *rdman_paint_image_new(redraw_man_t *rdman,
				      mb_img_data_t *img);
/*! \brief Set a matrix to transform image.
 */
extern void paint_image_set_matrix(paint_t *paint, co_aix matrix[6]);
extern void paint_image_get_size(paint_t *paint, int *w, int *h);

#endif /* __PAINT_H_ */
