// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __MB_GE_CAIRO_H_
#define __MB_GE_CAIRO_H_

#include <stdio.h>
#include <cairo.h>
#include "mb_basic_types.h"
#include "mb_img_ldr.h"

/*! \defgroup mb_ge_cairo MadButterfly Graphic Engine with Cairo
 * @{
 */
#define MBE_OPERATOR_CLEAR CAIRO_OPERATOR_CLEAR
#define MBE_OPERATOR_SOURCE CAIRO_OPERATOR_SOURCE
#define MBE_STATUS_SUCCESS CAIRO_STATUS_SUCCESS

/* This function is only used by img_ldr.c */
#define mbe_image_surface_create_from_png cairo_image_surface_create_from_png

#define mbe_pattern_create_for_surface cairo_pattern_create_for_surface
#define mbe_scaled_font_text_extents cairo_scaled_font_text_extents
#define mbe_image_surface_get_stride cairo_image_surface_get_stride
#define mbe_image_surface_get_height cairo_image_surface_get_height
#define mbe_directfb_surface_create cairo_directfb_surface_create
#define mbe_image_surface_get_width cairo_image_surface_get_width
#define mbe_image_surface_get_data cairo_image_surface_get_data
#define mbe_scaled_font_reference cairo_scaled_font_reference
#define mbe_win_surface_create cairo_xlib_surface_create
#define mbe_scaled_font_destroy cairo_scaled_font_destroy
#define mbe_font_face_reference cairo_font_face_reference
#define mbe_font_face_destroy cairo_font_face_destroy
#define mbe_paint_with_alpha cairo_paint_with_alpha
#define mbe_surface_destroy cairo_surface_destroy
#define mbe_set_source_rgba cairo_set_source_rgba
#define mbe_set_scaled_font cairo_set_scaled_font
#define mbe_pattern_destroy cairo_pattern_destroy
#define mbe_get_scaled_font cairo_get_scaled_font
#define mbe_set_source_rgb cairo_set_source_rgb
#define mbe_set_line_width cairo_set_line_width
#define mbe_get_font_face cairo_get_font_face
#define mbe_fill_preserve cairo_fill_preserve
#define mbe_set_source cairo_set_source
#define mbe_reset_scissoring cairo_reset_clip
#define mbe_get_target cairo_get_target
#define mbe_close_path cairo_close_path
#define mbe_text_path cairo_text_path
#define mbe_rectangle cairo_rectangle
#define mbe_in_stroke cairo_in_stroke
#define mbe_new_path cairo_new_path
#define mbe_curve_to cairo_curve_to
#define mbe_restore cairo_restore
#define mbe_move_to cairo_move_to
#define mbe_line_to cairo_line_to
#define mbe_in_fill cairo_in_fill
#define mbe_destroy cairo_destroy
#define mbe_stroke cairo_stroke
#define mbe_create cairo_create
#define mbe_paint cairo_paint
#define mbe_save cairo_save
#define mbe_fill cairo_fill
#define mbe_init()

typedef cairo_text_extents_t mbe_text_extents_t;
typedef cairo_scaled_font_t mbe_scaled_font_t;
typedef cairo_font_face_t mbe_font_face_t;
typedef cairo_surface_t mbe_surface_t;
typedef cairo_pattern_t mbe_pattern_t;
typedef cairo_t mbe_t;

#define MB_MATRIX_2_CAIRO(cmtx, mtx) {		\
	(cmtx).xx = (mtx)[0];			\
	(cmtx).xy = (mtx)[1];			\
	(cmtx).x0 = (mtx)[2];			\
	(cmtx).yx = (mtx)[3];			\
	(cmtx).yy = (mtx)[4];			\
	(cmtx).y0 = (mtx)[5];			\
}


extern mbe_font_face_t * mbe_query_font_face(const char *family,
					     int slant, int weight);
extern void mbe_free_font_face(mbe_font_face_t *face);
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
extern mbe_pattern_t *mbe_pattern_create_image(mb_img_data_t *img);
extern void mbe_scissoring(mbe_t *canvas, int n_areas, area_t **areas);
extern void mbe_copy_source(mbe_t *src, mbe_t *dst);


static void mbe_pattern_set_matrix(mbe_pattern_t *ptn,
				   const co_aix matrix[6]) {
    cairo_matrix_t cmtx;

    MB_MATRIX_2_CAIRO(cmtx, matrix);
    cairo_pattern_set_matrix(ptn, &cmtx);
}

static void mbe_clear(mbe_t *canvas) {
    cairo_operator_t old_op;

    old_op = cairo_get_operator(canvas);
    cairo_set_operator(canvas, CAIRO_OPERATOR_CLEAR);
    cairo_paint(canvas);
    cairo_set_operator(canvas, old_op);
}

static mbe_scaled_font_t *
mbe_scaled_font_create(mbe_font_face_t *face, co_aix fnt_mtx[6],
		       co_aix ctm[6]) {
    cairo_font_options_t *options;
    mbe_scaled_font_t *scaled;
    cairo_matrix_t cfnt_mtx, cctm;

    options = cairo_font_options_create();
    if(options == NULL)
	return NULL;

    MB_MATRIX_2_CAIRO(cfnt_mtx, fnt_mtx);
    MB_MATRIX_2_CAIRO(cctm, ctm);
    scaled = cairo_scaled_font_create(face, &cfnt_mtx, &cctm, options);

    cairo_font_options_destroy(options);

    return scaled;
}

static mbe_surface_t *
mbe_image_surface_create_for_data(unsigned char *data,
				  mb_img_fmt_t fmt,
				  int width, int height,
				  int stride) {
    cairo_format_t _fmt;

    switch(fmt) {
    case MB_IFMT_ARGB32:
	_fmt = CAIRO_FORMAT_ARGB32;
	break;
    case MB_IFMT_RGB24:
	_fmt = CAIRO_FORMAT_RGB24;
	break;
    case MB_IFMT_A8:
	_fmt = CAIRO_FORMAT_A8;
	break;
    case MB_IFMT_A1:
	_fmt = CAIRO_FORMAT_A1;
	break;
    default:
	return NULL;
    }
    return cairo_image_surface_create_for_data(data, _fmt,
					       width, height, stride);
}

static mb_img_fmt_t
mbe_image_surface_get_format(mbe_surface_t *surface) {
    cairo_format_t _fmt;
    mb_img_fmt_t fmt;

    _fmt = cairo_image_surface_get_format(surface);
    switch(_fmt) {
    case CAIRO_FORMAT_ARGB32:
	fmt = MB_IFMT_ARGB32;
	break;
    case CAIRO_FORMAT_RGB24:
	fmt = MB_IFMT_RGB24;
	break;
    case CAIRO_FORMAT_A8:
	fmt = MB_IFMT_A8;
	break;
    case CAIRO_FORMAT_A1:
	fmt = MB_IFMT_A1;
	break;
    default:
	fmt = MB_IFMT_DUMMY;
	break;
    }

    return fmt;
}

static mbe_surface_t *
mbe_image_surface_create(mb_img_fmt_t fmt, int width, int height) {
    cairo_format_t _fmt;

    switch(fmt) {
    case MB_IFMT_ARGB32:
	_fmt = CAIRO_FORMAT_ARGB32;
	break;
    case MB_IFMT_RGB24:
	_fmt = CAIRO_FORMAT_RGB24;
	break;
    case MB_IFMT_A8:
	_fmt = CAIRO_FORMAT_A8;
	break;
    case MB_IFMT_A1:
	_fmt = CAIRO_FORMAT_A1;
	break;
    default:
	return NULL;
    }

    return cairo_image_surface_create(_fmt, width, height);
}

static void
mbe_transform(mbe_t *mbe, const co_aix matrix[6]) {
    cairo_matrix_t cmtx;

    cmtx.xx = matrix[0];
    cmtx.xy = matrix[1];
    cmtx.x0 = matrix[2];
    cmtx.yx = matrix[3];
    cmtx.yy = matrix[4];
    cmtx.y0 = matrix[5];

    cairo_transform(mbe, &cmtx);
}

static void
mbe_arc(mbe_t *mbe, co_aix x, co_aix y, co_aix radius,
	co_aix angle_start, co_aix angle_stop) {
    if(angle_start <= angle_stop)
	cairo_arc(mbe, x, y, radius, angle_start, angle_stop);
    else
	cairo_arc_negative(mbe, x, y, radius, angle_start, angle_stop);
}
/* @} */

#endif /* __MB_GE_CAIRO_H_ */
