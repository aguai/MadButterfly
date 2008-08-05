#ifndef __SHAPES_H_
#define __SHAPES_H_

#include <cairo.h>
#include "mb_types.h"

/* Define a shape
 *
 * A shape must include
 * - *_new() and *_free()
 * - *_transform()
 * - *_draw()
 * - *_fill()
 * - *_stroke()
 * - struct of shape must include an shape_t as type of first member.
 */

/* TODO: remove *_fill() and *_stroke() */

extern void sh_path_free(shape_t *path);
extern shape_t *sh_path_new(char *data);
extern void sh_path_transform(shape_t *shape);
extern void sh_path_draw(shape_t *shape, cairo_t *cr);
extern void sh_path_fill(shape_t *shape, cairo_t *cr);
extern void sh_path_stroke(shape_t *shape, cairo_t *cr);


extern void sh_text_free(shape_t *text);
extern shape_t *sh_text_new(const char *txt, co_aix x, co_aix y,
			    co_aix font_size, cairo_font_face_t *face);
extern void sh_text_transform(shape_t *shape);
extern void sh_text_draw(shape_t *shape, cairo_t *cr);
extern void sh_text_fill(shape_t *shape, cairo_t *cr);
extern void sh_text_stroke(shape_t *shape, cairo_t *cr);


#endif /* __SHAPES_H_ */
