// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __MB_GE_SKIA_H_
#define __MB_GE_SKIA_H_

#include <stdio.h>
#include "mb_basic_types.h"
#include "mb_img_ldr.h"

/*! \defgroup mb_ge_skia MadButterfly Graphic Engine with Skia
 * @{
 */
#define MBE_OPERATOR_CLEAR 2
#define MBE_OPERATOR_SOURCE 1
#define MBE_STATUS_SUCCESS 0

struct _mbe_text_extents_t {
    co_aix x_bearing;
    co_aix y_bearing;
    co_aix width;
    co_aix height;
    co_aix x_advance;
    co_aix y_advance;
};
struct _mbe_scaled_font_t;
struct _mbe_font_face_t;
struct _mbe_surface_t;
struct _mbe_pattern_t;
struct _mbe_t;

typedef struct _mbe_text_extents_t mbe_text_extents_t;
typedef struct _mbe_scaled_font_t mbe_scaled_font_t;
typedef struct _mbe_font_face_t mbe_font_face_t;
typedef struct _mbe_surface_t mbe_surface_t;
typedef struct _mbe_pattern_t mbe_pattern_t;
typedef struct _mbe_t mbe_t;

extern mbe_pattern_t *mbe_pattern_create_for_surface(mbe_surface_t *surface);
extern mbe_pattern_t *mbe_pattern_create_radial(co_aix cx0, co_aix cy0,
						co_aix radius0,
						co_aix cx1, co_aix cy1,
						co_aix radius1,
						grad_stop_t *stops,
						int stop_cnt);
extern mbe_pattern_t *mbe_pattern_create_linear(co_aix x0, co_aix y0,
						co_aix x1, co_aix y1,
						grad_stop_t *stops,
						int stop_cnt);
extern void mbe_pattern_set_matrix(mbe_pattern_t *ptn,
				   const co_aix matrix[6]);
extern void mbe_pattern_destroy(mbe_pattern_t *ptn);

extern int mbe_image_surface_get_stride(mbe_surface_t *surface);
extern int mbe_image_surface_get_height(mbe_surface_t *surface);
extern int mbe_image_surface_get_width(mbe_surface_t *surface);
extern unsigned char *mbe_image_surface_get_data(mbe_surface_t *surface);
extern mbe_surface_t *mbe_image_surface_create_from_png(const char *filename);
extern mbe_surface_t *
mbe_image_surface_create_for_data(unsigned char *data,
				  mb_img_fmt_t fmt,
				  int width, int height,
				  int stride);
extern mb_img_fmt_t mbe_image_surface_get_format(mbe_surface_t *surface);
extern mbe_surface_t *
mbe_image_surface_create(mb_img_fmt_t fmt, int width, int height);

extern mbe_scaled_font_t *mbe_scaled_font_reference(mbe_scaled_font_t *scaled);
extern void mbe_scaled_font_destroy(mbe_scaled_font_t *scaled);
extern mbe_font_face_t *mbe_font_face_reference(mbe_font_face_t *face);
extern mbe_scaled_font_t *
mbe_scaled_font_create(mbe_font_face_t *face, co_aix fnt_mtx[6],
		       co_aix ctm[6]);
extern mbe_scaled_font_t *mbe_get_scaled_font(mbe_t *canvas);
extern void mbe_scaled_font_text_extents(mbe_scaled_font_t *scaled,
					 const char *txt,
					 mbe_text_extents_t *extents);

extern void mbe_font_face_destroy(mbe_font_face_t *face);
extern void mbe_paint_with_alpha(mbe_t *canvas, co_aix alpha);
extern void mbe_surface_destroy(mbe_surface_t *surface);
extern void mbe_set_source_rgba(mbe_t *canvas,
				co_aix r, co_aix g, co_aix b, co_aix a);
extern void mbe_set_scaled_font(mbe_t *canvas,
				const mbe_scaled_font_t *scaled);
extern void mbe_set_source_rgb(mbe_t *canvas, co_aix r, co_aix g, co_aix b);
extern void mbe_set_line_width(mbe_t *canvas, co_aix width);
extern mbe_font_face_t *mbe_get_font_face(mbe_t *canvas);
extern void mbe_fill_preserve(mbe_t *canvas);
extern void mbe_set_source(mbe_t *canvas, mbe_pattern_t *source);
extern void mbe_reset_clip(mbe_t *canvas);
extern mbe_surface_t *mbe_get_target(mbe_t *canvas);
extern void mbe_close_path(mbe_t *canvas);
extern void mbe_text_path(mbe_t *canvas, const char *txt);
extern void mbe_rectangle(mbe_t *canvas, co_aix x, co_aix y,
			  co_aix width, co_aix height);
extern int mbe_in_stroke(mbe_t *canvas, co_aix x, co_aix y);
extern void mbe_new_path(mbe_t *canvas);
extern void mbe_curve_to(mbe_t *canvas, co_aix x1, co_aix y1,
			 co_aix x2, co_aix y2,
			 co_aix x3, co_aix y3);
extern void mbe_restore(mbe_t *canvas);
extern void mbe_move_to(mbe_t *canvas, co_aix x, co_aix y);
extern void mbe_line_to(mbe_t *canvas, co_aix x, co_aix y);
extern int mbe_in_fill(mbe_t *canvas, co_aix x, co_aix y);
extern void mbe_stroke(mbe_t *canvas);
extern mbe_t *mbe_create(mbe_surface_t *target);
extern void mbe_destroy(mbe_t *canvas);
extern void mbe_paint(mbe_t *canvas);
extern void mbe_save(mbe_t *canvas);
extern void mbe_fill(mbe_t *canvas);
extern void mbe_clip(mbe_t *canvas);

extern mbe_font_face_t * mbe_query_font_face(const char *family,
					     int slant, int weight);
extern void mbe_free_font_face(mbe_font_face_t *face);

extern void mbe_clear(mbe_t *canvas);
extern void mbe_copy_source(mbe_t *src, mbe_t *dst);
extern void mbe_transform(mbe_t *mbe, co_aix matrix[6]);
extern void mbe_arc(mbe_t *mbe, co_aix x, co_aix y, co_aix radius,
		    co_aix angle_start, co_aix angle_stop);
/* @} */

#endif /* __MB_GE_SKIA_H_ */
