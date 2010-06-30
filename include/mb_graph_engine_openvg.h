#ifndef __MB_GE_OPENVG_H_
#define __MB_GE_OPENVG_H_

#include <stdio.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <VG/openvg.h>
#include "mb_basic_types.h"
#include "mb_img_ldr.h"

/*! \defgroup mb_ge_cairo MadButterfly Graphic Engine with Cairo
 * @{
 */
#define MBE_OPERATOR_CLEAR OPENVG_OPERATOR_CLEAR
#define MBE_OPERATOR_SOURCE OPENVG_OPERATOR_SOURCE
#define MBE_STATUS_SUCCESS OPENVG_STATUS_SUCCESS

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
#define mbe_pattern_create_radial(cx0, cy0, radius0,			\
				  cx1, cy1, radius1, stops, stop_cnt)	\
    ((mbe_pattern_t *)NULL)
#define mbe_pattern_create_linear(x0, y0, x1, y1, stops, stop_cnt)	\
    ((mbe_pattern_t *)NULL)
#define mbe_scaled_font_destroy(scaled)
#define mbe_font_face_reference(face) ((mbe_font_face_t *)NULL)
#define mbe_scaled_font_create(face, fnt_mtx, ctm) ((mbe_scaled_font_t *)NULL)
#define mbe_pattern_set_matrix(ptn, mtx)
#define mbe_font_face_destroy(face)
#define mbe_paint_with_alpha(canvas, alpha)
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
#define mbe_set_source(canvas, pattern)		\
    do { (canvas)->src = (pattern); } while(0)
#define mbe_reset_scissoring(canvas)
#define mbe_get_target(canvas) ((mbe_surface_t *)(canvas)->tgt)
#define mbe_close_path(canvas)
#define mbe_text_path(canvas, utf8)
#define mbe_rectangle(canvas, x, y, w, h)
#define mbe_in_stroke(canvas, x, y) (0)
#define mbe_new_path(canvas)
#define mbe_curve_to(canvas, x1, y1, x2, y2, x3, y3)
#define mbe_restore(canvas)
#define mbe_move_to(canvas, x, y)
#define mbe_line_to(canvas, x, y)
#define mbe_in_fill(canvas, x, y) (0)
#define mbe_clear(canvas)
#define mbe_paint(canvas)
#define mbe_save(canvas)
#define mbe_arc(canvas, x, y, radius, angle_start, angle_stop)

typedef struct _mbe_text_extents_t mbe_text_extents_t;
typedef int mbe_scaled_font_t;
typedef int mbe_font_face_t;
typedef struct _ge_openvg_surface mbe_surface_t;
typedef struct _ge_openvg_pattern mbe_pattern_t;
typedef struct _ge_openvg_mbe mbe_t;

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
    mbe_surface_t *tgt;
    EGLContext ctx;
    VGPath path;
};

struct _ge_openvg_surface {
    void *surface;
    mbe_t *asso_mbe;		/* There is a association between
				 * surface and mbe */
    int w, h;
};

struct _ge_openvg_pattern {
    void *pattern;
    void *asso_img;
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

extern void mbe_scissoring(mbe_t *canvas, int n_areas, area_t **areas);


#define _VG_DISPLAY() eglGetDisplay(_ge_openvg_disp_id)

/* \brief Make the context of a canvas to be current context.
 */
#define _MK_CURRENT_CTX(canvas) do {				\
	if(_ge_openvg_current_canvas != (canvas)) {		\
	    _ge_openvg_current_canvas = canvas;			\
	    eglMakeCurrent(_VG_DISPLAY(), (canvas)->tgt,	\
			   (canvas)->tgt, (canvas)->ctx);	\
	}							\
    } while(0)

static void
mbe_transform(mbe_t *canvas, co_aix *mtx) {
    VGfloat vg_mtx[9];
    
    _MK_CURRENT_CTX(canvas);
    MB_MATRIX_2_OPENVG(vg_mtx, mtx);
    vgLoadMatrix(vg_mtx);
}

#define EGL_GLX 1
#ifdef EGL_GLX
#include <X11/Xlib.h>
#include <X11/Xutil.h>

extern mbe_surface_t *mbe_vg_win_surface_create(Display *display,
						Drawable drawable,
						Visual *visual,
						int width, int height);
#endif

extern mbe_surface_t *mbe_image_surface_create(mb_img_fmt_t fmt,
					       int w, int h);

static void
mbe_surface_destroy(mbe_surface_t *surface) {
    EGLDisplay display;

    display = _VG_DISPLAY();
    eglDestroySurface(display, surface->surface);
    
    if(surface->asso_mbe)
	surface->asso_mbe->tgt = NULL;
    
    free(surface);
}

extern mbe_t *mbe_create(mbe_surface_t *surface);
extern void mbe_destroy(mbe_t *canvas);
static void
mbe_stroke(mbe_t *canvas) {
    _MK_CURRENT_CTX(canvas);

    vgDrawPath(canvas->path, VG_STROKE_PATH);
    vgClearPath(canvas->path, VG_PATH_CAPABILITY_ALL);
}

static void
mbe_fill(mbe_t *canvas) {
    _MK_CURRENT_CTX(canvas);

    vgDrawPath(canvas->path, VG_FILL_PATH);
    vgClearPath(canvas->path, VG_PATH_CAPABILITY_ALL);
}

/* @} */

#endif /* __MB_GE_OPENVG_H_ */
