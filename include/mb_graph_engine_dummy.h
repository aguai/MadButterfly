/* This is a dummy Graphic Engine to consume all graphic operators.
 */
#ifndef __MB_GE_OPENVG_H_
#define __MB_GE_OPENVG_H_

#include <stdio.h>
#include <GL/glut.h>
#include "mb_basic_types.h"
#include "mb_img_ldr.h"

/*! \defgroup mb_ge_cairo MadButterfly Graphic Engine with Cairo
 * @{
 */
#define mbe_image_surface_create_from_png(fn) ((mbe_surface_t *)NULL)
#define mbe_image_surface_create_for_data(data, fmt, w, h, stride)	\
    ((mbe_surface_t *)NULL)
#define mbe_pattern_create_for_surface(canvas) ((mbe_pattern_t *)NULL)
#define mbe_scaled_font_text_extents(scaled, utf8, extents)
#define mbe_image_surface_get_stride(surface) (20)
#define mbe_image_surface_get_format(surface) ((mb_img_fmt_t)0)
#define mbe_image_surface_get_height(surface) (1)
#define mbe_image_surface_get_width(surface) (1)
#define mbe_image_surface_get_data(surface) ((unsigned char *)NULL)
#define mbe_scaled_font_reference(scaled) ((mbe_scaled_font_t *)NULL)
/* For OpenVG backend, never invoke xlib surface.
 * #define mbe_xlib_surface_create cairo_xlib_surface_create
 */
#define mbe_pattern_create_radial(cx0, cy0, radius0,			\
				  cx1, cy1, radius1, stops, stop_cnt)	\
    ((mbe_pattern_t *)NULL)
#define mbe_pattern_create_linear(x0, y0, x1, y1, stops, stop_cnt)	\
    ((mbe_pattern_t *)NULL)
#define mbe_image_surface_create(fmt, w, h) ((mbe_surface_t *)NULL)
#define mbe_scaled_font_destroy(scaled)
#define mbe_font_face_reference(face) ((mbe_font_face_t *)NULL)
#define mbe_scaled_font_create(face, fnt_mtx, ctm) ((mbe_scaled_font_t *)NULL)
#define mbe_pattern_set_matrix(ptn, mtx)
#define mbe_font_face_destroy(face)
#define mbe_paint_with_alpha(canvas, alpha)
#define mbe_surface_destroy(surface)
#define mbe_set_source_rgba(canvas, r, g, b, a)
#define mbe_set_scaled_font(canvas, scaled)
#define mbe_pattern_destroy(pattern)
#define mbe_get_scaled_font(canvas) ((mbe_scaled_font_t *)NULL)
#define mbe_query_font_face(family, slant, weight) ((mbe_font_face_t *)NULL)
#define mbe_free_font_face(face)
#define mbe_set_line_width(canvas, w)
#define mbe_set_source_rgb(canvas, r, g, b)
#define mbe_get_font_face(canvas) ((mbe_font_face_t *)NULL)
#define mbe_fill_preserve(canvas)
#define mbe_copy_source(src_canvas, dst_canvas)
#define mbe_set_source(canvas, pattern)
#define mbe_reset_scissoring(canvas)
#define mbe_get_target(canvas) ((mbe_surface_t *)NULL)
#define mbe_close_path(canvas)
#define mbe_text_path(canvas, utf8)
#define mbe_transform(canvas, mtx)
#define mbe_rectangle(canvas, x, y, w, h)
#define mbe_in_stroke(canvas, x, y) (0)
#define mbe_new_path(canvas)
#define mbe_curve_to(canvas, x1, y1, x2, y2, x3, y3)
#define mbe_restore(canvas)
#define mbe_move_to(canvas, x, y)
#define mbe_line_to(canvas, x, y)
#define mbe_in_fill(canvas, x, y) (0)
#define mbe_destroy(canvas)
#define mbe_stroke(canvas)
#define mbe_create(surface) ((mbe_t *)NULL)
#define mbe_clear(canvas)
#define mbe_paint(canvas)
#define mbe_save(canvas)
#define mbe_fill(canvas)
/*! \brief Make scissoring rectangles.
 *
 * It would reset all previous pathes.
 */
#define mbe_scissoring(canvas, n_areas, areas)
#define mbe_arc(canvas, x, y, radius, angle_start, angle_stop)

typedef struct _mbe_text_extents_t mbe_text_extents_t;
typedef int mbe_scaled_font_t;
typedef int mbe_font_face_t;
typedef int mbe_surface_t;
typedef int mbe_pattern_t;
typedef int mbe_t;

struct _mbe_text_extents_t {
    co_aix x_bearing;
    co_aix y_bearing;
    co_aix width;
    co_aix height;
    co_aix x_advance;
    co_aix y_advance;
};

/* @} */

#endif /* __MB_GE_OPENVG_H_ */
