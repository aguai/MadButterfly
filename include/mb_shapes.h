/*! \file
 * \brief Declare interfaces of shapes.
 *
 * \todo Add ellipse shape.
 * \todo Add circle shape.
 */
#ifndef __SHAPES_H_
#define __SHAPES_H_

#include <cairo.h>
#include "mb_types.h"
#include "mb_redraw_man.h"

/*! \page define_shape How to Define Shapes
 *
 * A shape implementation must include
 * - rdman_shape_*_new()
 *   - clear memory for shape_t member.
 *   - assign *_free() to \ref shape_t::free.
 *   - make new object been managed by a redraw manager.
 *     - call rdman_shape_man()
 * - *_free()
 *   - assigned to \ref shape_t::free.
 * - *_transform()
 * - *_draw()
 * - first member variable of a shape type must be a shape_t.
 * 
 * Must modify
 * - event.c::draw_shape_path()
 * - redraw_man.c::clean_shape()
 * - redraw_man.c::draw_shape()
 */

/*! \defgroup shapes Shapes
 * @{
 */

/*! \defgroup shape_path Shape of Path
 * @{
 */
extern shape_t *rdman_shape_path_new(redraw_man_t *rdman, char *data);
extern shape_t *rdman_shape_path_new_from_binary(redraw_man_t *rdman, char *commands, co_aix *arg,int  arg_cnt,int *fix_arg,int fix_arg_cnt);
extern void sh_path_transform(shape_t *shape);
extern void sh_path_draw(shape_t *shape, cairo_t *cr);
/* @} */

/*! \defgroup shape_text Shape of Text
 * @{
 */
extern shape_t *rdman_shape_text_new(redraw_man_t *rdman,
				     const char *txt, co_aix x, co_aix y,
				     co_aix font_size,
				     cairo_font_face_t *face);
extern void sh_text_set_text(shape_t *shape, const char *txt);
extern void sh_text_transform(shape_t *shape);
extern void sh_text_draw(shape_t *shape, cairo_t *cr);
/* @} */

/*! \defgroup shape_rect Shape of Rectangle
 * @{
 */
extern shape_t *rdman_shape_rect_new(redraw_man_t *rdman,
				     co_aix x, co_aix y,
				     co_aix w, co_aix h,
				     co_aix rx, co_aix ry);
extern void sh_rect_transform(shape_t *shape);
extern void sh_rect_draw(shape_t *shape, cairo_t *cr);
extern void sh_rect_set(shape_t *shape, co_aix x, co_aix y,
			co_aix w, co_aix h, co_aix rx, co_aix ry);
/* @} */
/* @} */

#endif /* __SHAPES_H_ */
