#ifndef __MB_GE_OPENVG_H_
#define __MB_GE_OPENVG_H_

#include <stdio.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <VG/openvg.h>
#include "mb_basic_types.h"
#include "mb_img_ldr.h"

/*! \defgroup mb_ge_openvg MadButterfly Graphic Engine with OpenVG
 * @{
 */
#define mbe_scaled_font_text_extents(scaled, utf8, extents)
#define mbe_image_surface_get_stride(surface) ((surface)->w * 4)
#define mbe_image_surface_get_format(surface) ((surface)->fmt)
#define mbe_image_surface_get_height(surface) (surface)->h
#define mbe_image_surface_get_width(surface) (surface)->w
#define mbe_image_surface_get_data(surface) ((unsigned char *)NULL)
#define mbe_scaled_font_reference(scaled) ((mbe_scaled_font_t *)NULL)
#define mbe_scaled_font_destroy(scaled)
#define mbe_font_face_reference(face) ((mbe_font_face_t *)NULL)
#define mbe_scaled_font_create(face, fnt_mtx, ctm) ((mbe_scaled_font_t *)NULL)
#define mbe_font_face_destroy(face)
#define mbe_set_scaled_font(canvas, scaled)
#define mbe_get_scaled_font(canvas) ((mbe_scaled_font_t *)NULL)
#define mbe_query_font_face(family, slant, weight) ((mbe_font_face_t *)NULL)
#define mbe_free_font_face(face)
#define mbe_set_line_width(canvas, w)		\
    do {					\
	_MK_CURRENT_CTX(canvas);		\
	vgSetf(VG_STROKE_LINE_WIDTH, w);	\
    } while(0)
#define mbe_set_source_rgb(canvas, r, g, b)	\
    mbe_set_source_rgba(canvas, r, g, b, 1)
#define mbe_get_font_face(canvas) ((mbe_font_face_t *)NULL)
#define mbe_set_source(canvas, pattern)		\
    do {					\
	(canvas)->src = (pattern);		\
	(canvas)->paint = (pattern)->paint;	\
	(canvas)->paint_installed = 0;		\
    } while(0)
#define mbe_reset_scissoring(canvas)		\
    do {					\
	_MK_CURRENT_CTX(canvas);		\
	vgSeti(VG_SCISSORING, VG_FALSE);	\
    } while(0)
#define mbe_get_target(canvas) ((mbe_surface_t *)(canvas)->tgt)
#define mbe_close_path(canvas)			\
    do {								\
	char _vg_cmd = VG_CLOSE_PATH;					\
	vgAppendPathData((canvas)->path, 1, &_vg_cmd, NULL);		\
    } while(0)
#define mbe_text_path(canvas, utf8)
#define mbe_rectangle(canvas, x, y, w, h)
#define mbe_in_stroke(canvas, x, y) (0)
#define mbe_new_path(canvas)				\
    vgClearPath((canvas)->path, VG_PATH_CAPABILITY_ALL)
#define mbe_curve_to(canvas, x1, y1, x2, y2, x3, y3)			\
    do {								\
	VGfloat _vg_data[6] = {x1, y1, x2, y2, x3, y3};			\
	char _vg_cmd = VG_CUBIC_TO_ABS;					\
	vgAppendPathData((canvas)->path, 1, &_vg_cmd, _vg_data);	\
    } while(0)
#define mbe_move_to(canvas, x, y)					\
    do {								\
	VGfloat _vg_data[2] = {x, y};					\
	char _vg_cmd = VG_MOVE_TO_ABS;					\
	vgAppendPathData((canvas)->path, 1, &_vg_cmd, _vg_data);	\
    } while(0)
#define mbe_line_to(canvas, x, y)					\
    do {								\
	VGfloat _vg_data[2] = {x, y};					\
	char _vg_cmd = VG_LINE_TO_ABS;					\
	vgAppendPathData((canvas)->path, 1, &_vg_cmd, _vg_data);	\
    } while(0)
#define mbe_in_fill(canvas, x, y) (1)
/* TODO: change prototype of mbe_arc() to remove mbe_save() and
 *	 mbe_restore().
 */
#define mbe_save(canvas)
#define mbe_restore(canvas)
#define mbe_arc(canvas, x, y, radius, angle_start, angle_stop)

typedef struct _mbe_text_extents_t mbe_text_extents_t;
typedef int mbe_scaled_font_t;
typedef int mbe_font_face_t;
typedef struct _ge_openvg_surface mbe_surface_t;
typedef struct _ge_openvg_pattern mbe_pattern_t;
typedef struct _ge_openvg_mbe mbe_t;
typedef struct _ge_openvg_img _ge_openvg_img_t;

struct _mbe_text_extents_t {
    co_aix x_bearing;
    co_aix y_bearing;
    co_aix width;
    co_aix height;
    co_aix x_advance;
    co_aix y_advance;
};

struct _ge_openvg_mbe {
    mbe_pattern_t *src;
    VGPaint paint;		/*!< \brief The paint associated with
				 * the src pattern */
    int paint_installed;
    mbe_surface_t *tgt;
    EGLContext ctx;
    VGPath path;

    VGfloat mtx[9];
};

struct _ge_openvg_surface {
    void *surface;
    mbe_t *asso_mbe;		/* There is a association between
				 * surface and mbe */
    _ge_openvg_img_t *asso_img;
    int w, h;
    mb_img_fmt_t fmt;
};

struct _ge_openvg_pattern {
    _ge_openvg_img_t *asso_img;
    VGfloat mtx[9];
    VGPaint paint;
};

#define MB_MATRIX_2_OPENVG(vgmtx, mtx) do {	\
	(vgmtx)[0] = (mtx)[0];			\
	(vgmtx)[1] = (mtx)[1];			\
	(vgmtx)[2] = (mtx)[2];			\
	(vgmtx)[3] = (mtx)[3];			\
	(vgmtx)[4] = (mtx)[4];			\
	(vgmtx)[5] = (mtx)[5];			\
	(vgmtx)[6] = 0;				\
	(vgmtx)[7] = 0;				\
	(vgmtx)[8] = 1;				\
    } while(0)

extern EGLNativeDisplayType _ge_openvg_disp_id;
extern mbe_t *_ge_openvg_current_canvas;
extern void _mbe_load_pattern_mtx(VGfloat *mtx1, VGfloat *mtx2, int mode);
extern void _ge_vg_img_activate_for_pattern(mbe_pattern_t *ptn);
extern void _ge_vg_img_activate_for_surface(mbe_surface_t *surf);

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
extern mbe_pattern_t *mbe_pattern_create_image(mb_img_data_t *img);
extern void mbe_pattern_destroy(mbe_pattern_t *ptn);
extern void mbe_pattern_set_matrix(mbe_pattern_t *ptn, co_aix *mtx);
extern void mbe_set_source_rgba(mbe_t *canvas, co_comp_t r, co_comp_t g,
				co_comp_t b, co_comp_t a);
/* TODO: rename n_areas to areas_cnt and make it after areas */
extern void mbe_scissoring(mbe_t *canvas, int n_areas, area_t **areas);


#define _VG_DISPLAY() eglGetDisplay(_ge_openvg_disp_id)

/* \brief Make the context of a canvas to be current context.
 *
 * TODO: swtich VGImage between VGPaint and Surface.
 */
#define _MK_CURRENT_CTX(canvas) do {				\
	if(_ge_openvg_current_canvas != (canvas)) {		\
	    _ge_openvg_current_canvas = canvas;			\
	    eglMakeCurrent(_VG_DISPLAY(),			\
			   (canvas)->tgt->surface,		\
			   (canvas)->tgt->surface,		\
			   (canvas)->ctx);			\
	}							\
	/* \sa _ge_openvg_img_t */				\
	_ge_vg_img_activate_for_surface((canvas)->tgt);		\
    } while(0)
/* TODO: switch VGImage between VGPaint and surface. */
#define _MK_CURRENT_PAINT(canvas)					\
    do {								\
	if((canvas)->paint_installed == 0) {				\
	    vgSetPaint((canvas)->paint, VG_FILL_PATH|VG_STROKE_PATH);	\
	    (canvas)->paint_installed = 1;				\
	}								\
	/* \sa _ge_openvg_img_t */					\
	if((canvas)->src)						\
	    _ge_vg_img_activate_for_pattern((canvas)->src);		\
    } while(0)

#define mbe_transform(canvas, _mtx)				\
    do {							\
	_MK_CURRENT_CTX(canvas);				\
	MB_MATRIX_2_OPENVG((canvas)->mtx, _mtx);		\
	_mbe_load_pattern_mtx(_mtx, NULL,			\
			      VG_MATRIX_PATH_USER_TO_SURFACE);	\
    } while(0)


#define EGL_GLX 1
#ifdef EGL_GLX
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*
 * TODO: define a proper type for display and drawable.
 */
extern mbe_surface_t *mbe_win_surface_create(void *display,
					     void *drawable,
					     int fmt,
					     int width, int height);
#endif

extern mbe_surface_t *mbe_image_surface_create(mb_img_fmt_t fmt,
					       int w, int h);
extern mbe_surface_t *
mbe_image_surface_create_for_data(unsigned char *data,
				  mb_img_fmt_t fmt,
				  int width, int height,
				  int stride);
extern void mbe_surface_destroy(mbe_surface_t *surface);

extern void mbe_copy_source(mbe_t *src_canvas, mbe_t *dst_canvas);
extern void mbe_flush(mbe_t *canvas);
extern mbe_t *mbe_create(mbe_surface_t *surface);
extern void mbe_destroy(mbe_t *canvas);
extern void mbe_paint_with_alpha(mbe_t *canvas, co_comp_t alpha);
extern void mbe_paint(mbe_t *canvas);
extern void mbe_clear(mbe_t *canvas);
extern void mbe_init();

static void
mbe_stroke(mbe_t *canvas) {
    _MK_CURRENT_CTX(canvas);
    _MK_CURRENT_PAINT(canvas);
    if(canvas->src)
	_mbe_load_pattern_mtx(canvas->src->mtx, NULL,
			      VG_MATRIX_STROKE_PAINT_TO_USER);

    vgDrawPath(canvas->path, VG_STROKE_PATH);
    vgClearPath(canvas->path, VG_PATH_CAPABILITY_ALL);
}

static void
mbe_fill_preserve(mbe_t *canvas) {
    _MK_CURRENT_CTX(canvas);
    _MK_CURRENT_PAINT(canvas);
    if(canvas->src)
	_mbe_load_pattern_mtx(canvas->src->mtx, NULL,
			      VG_MATRIX_FILL_PAINT_TO_USER);

    vgDrawPath(canvas->path, VG_FILL_PATH);
}

static void
mbe_fill(mbe_t *canvas) {
    mbe_fill_preserve(canvas);
    vgClearPath(canvas->path, VG_PATH_CAPABILITY_ALL);
}
/* @} */

#endif /* __MB_GE_OPENVG_H_ */
