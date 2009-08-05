#ifndef __MBE_H_
#define __MBE_H_

#include <stdio.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include "mb_img_ldr.h"

/*! \defgroup mb_graph_engine MadButterfly Graphic Engine
 * @{
 */
#define MBE_OPERATOR_CLEAR CAIRO_OPERATOR_CLEAR
#define MBE_OPERATOR_SOURCE CAIRO_OPERATOR_SOURCE
#define MBE_STATUS_SUCCESS CAIRO_STATUS_SUCCESS

#define mbe_ft_font_face_create_for_pattern cairo_ft_font_face_create_for_pattern
#define mbe_image_surface_create_from_png cairo_image_surface_create_from_png
#define mbe_pattern_add_color_stop_rgba cairo_pattern_add_color_stop_rgba
#define mbe_pattern_create_for_surface cairo_pattern_create_for_surface
#define mbe_scaled_font_text_extents cairo_scaled_font_text_extents
#define mbe_image_surface_get_stride cairo_image_surface_get_stride
#define mbe_image_surface_get_height cairo_image_surface_get_height
#define mbe_image_surface_get_width cairo_image_surface_get_width
#define mbe_image_surface_get_data cairo_image_surface_get_data
#define mbe_scaled_font_reference cairo_scaled_font_reference
#define mbe_pattern_create_radial cairo_pattern_create_radial
#define mbe_pattern_create_linear cairo_pattern_create_linear
#define mbe_xlib_surface_create cairo_xlib_surface_create
#define mbe_scaled_font_destroy cairo_scaled_font_destroy
#define mbe_font_options_create cairo_font_options_create
#define mbe_font_face_reference cairo_font_face_reference
#define mbe_set_source_surface cairo_set_source_surface
#define mbe_scaled_font_status cairo_scaled_font_status
#define mbe_scaled_font_create cairo_scaled_font_create
#define mbe_pattern_set_matrix cairo_pattern_set_matrix
#define mbe_font_face_destroy cairo_font_face_destroy
#define mbe_paint_with_alpha cairo_paint_with_alpha
#define mbe_font_face_status cairo_font_face_status
#define mbe_surface_destroy cairo_surface_destroy
#define mbe_set_source_rgba cairo_set_source_rgba
#define mbe_set_scaled_font cairo_set_scaled_font
#define mbe_pattern_destroy cairo_pattern_destroy
#define mbe_get_scaled_font cairo_get_scaled_font
#define mbe_set_source_rgb cairo_set_source_rgb
#define mbe_set_line_width cairo_set_line_width
#define mbe_get_font_face cairo_get_font_face
#define mbe_fill_preserve cairo_fill_preserve
#define mbe_set_operator cairo_set_operator
#define mbe_get_operator cairo_get_operator
#define mbe_arc_negative cairo_arc_negative
#define mbe_set_source cairo_set_source
#define mbe_reset_clip cairo_reset_clip
#define mbe_get_target cairo_get_target
#define mbe_close_path cairo_close_path
#define mbe_translate cairo_translate
#define mbe_text_path cairo_text_path
#define mbe_show_text cairo_show_text
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
#define mbe_rotate cairo_rotate
#define mbe_create cairo_create
#define mbe_scale cairo_scale
#define mbe_paint cairo_paint
#define mbe_save cairo_save
#define mbe_fill cairo_fill
#define mbe_clip cairo_clip
#define mbe_arc cairo_arc

typedef cairo_text_extents_t mbe_text_extents_t;
typedef cairo_font_options_t mbe_font_options_t;
typedef cairo_scaled_font_t mbe_scaled_font_t;
typedef cairo_font_face_t mbe_font_face_t;
typedef cairo_operator_t mbe_operator_t;
typedef cairo_surface_t mbe_surface_t;
typedef cairo_pattern_t mbe_pattern_t;
typedef cairo_status_t mbe_status_t;
typedef cairo_matrix_t mbe_matrix_t;
typedef cairo_t mbe_t;

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

/* @} */

#endif /* __MBE_H_ */
