#ifndef __SHAPES_H_
#define __SHAPES_H_

#include <cairo.h>
#include "mb_types.h"

/* Define a shape
 *
 * A shape must include
 * - *_new() and *_free()
 *   - clear memory for shape_t member.
 * - *_transform()
 * - *_draw()
 * - struct of shape must include an shape_t as type of first member.
 * 
 * Must modify
 * - event.c:draw_shape_path
 * - redraw_man.c:clean_shape
 * - redraw_man.c:draw_shape
 */


extern void sh_path_free(shape_t *path);
extern shape_t *sh_path_new(char *data);
extern void sh_path_transform(shape_t *shape);
extern void sh_path_draw(shape_t *shape, cairo_t *cr);


extern void sh_text_free(shape_t *text);
extern shape_t *sh_text_new(const char *txt, co_aix x, co_aix y,
			    co_aix font_size, cairo_font_face_t *face);
extern void sh_text_transform(shape_t *shape);
extern void sh_text_draw(shape_t *shape, cairo_t *cr);

extern shape_t *sh_rect_new(co_aix x, co_aix y, co_aix w, co_aix h,
			    co_aix rx, co_aix ry);
extern void sh_rect_free(shape_t *shape);
extern void sh_rect_transform(shape_t *shape);
extern void sh_rect_draw(shape_t *shape, cairo_t *cr);
extern void sh_rect_set(shape_t *shape, co_aix x, co_aix y,
			co_aix w, co_aix h, co_aix rx, co_aix ry);

#endif /* __SHAPES_H_ */
