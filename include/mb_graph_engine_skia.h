#ifndef __MB_GE_SKIA_H_
#define __MB_GE_SKIA_H_

#include <stdio.h>
#include "mb_img_ldr.h"

/*! \defgroup mb_ge_skia MadButterfly Graphic Engine with Skia
 * @{
 */
#define MBE_OPERATOR_CLEAR CAIRO_OPERATOR_CLEAR
#define MBE_OPERATOR_SOURCE CAIRO_OPERATOR_SOURCE
#define MBE_STATUS_SUCCESS CAIRO_STATUS_SUCCESS

#define mbe_ft_font_face_create_for_pattern
#define mbe_image_surface_create_from_png
#define mbe_pattern_add_color_stop_rgba
#define mbe_pattern_create_for_surface
#define mbe_scaled_font_text_extents
#define mbe_image_surface_get_stride
#define mbe_image_surface_get_height
#define mbe_image_surface_get_width
#define mbe_image_surface_get_data
#define mbe_scaled_font_reference
#define mbe_pattern_create_radial
#define mbe_pattern_create_linear
#define mbe_xlib_surface_create
#define mbe_scaled_font_destroy
#define mbe_font_face_reference
#define mbe_set_source_surface
#define mbe_scaled_font_status
#define mbe_scaled_font_create
#define mbe_pattern_set_matrix
#define mbe_font_face_destroy
#define mbe_paint_with_alpha
#define mbe_font_face_status
#define mbe_surface_destroy
#define mbe_set_source_rgba
#define mbe_set_scaled_font
#define mbe_pattern_destroy
#define mbe_get_scaled_font
#define mbe_set_source_rgb
#define mbe_set_line_width
#define mbe_get_font_face
#define mbe_fill_preserve
#define mbe_set_operator
#define mbe_get_operator
#define mbe_set_source
#define mbe_reset_clip
#define mbe_get_target
#define mbe_close_path
#define mbe_text_path
#define mbe_show_text
#define mbe_rectangle
#define mbe_in_stroke
#define mbe_new_path
#define mbe_curve_to
#define mbe_restore
#define mbe_move_to
#define mbe_line_to
#define mbe_in_fill
#define mbe_destroy
#define mbe_stroke
#define mbe_create
#define mbe_paint
#define mbe_save
#define mbe_fill
#define mbe_clip

typedef cairo_text_extents_t mbe_text_extents_t;
typedef cairo_scaled_font_t mbe_scaled_font_t;
typedef cairo_font_face_t mbe_font_face_t;
typedef cairo_operator_t mbe_operator_t;
typedef cairo_surface_t mbe_surface_t;
typedef cairo_pattern_t mbe_pattern_t;
typedef cairo_status_t mbe_status_t;
typedef cairo_matrix_t mbe_matrix_t;
typedef cairo_t mbe_t;
typedef float co_aix;

extern mbe_font_face_t * mbe_query_font_face(const char *family,
					     int slant, int weight);
extern void mbe_free_font_face(mbe_font_face_t *face);

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

#endif /* __MB_GE_SKIA_H_ */
