#include "mb_graph_engine_openvg.h"

EGLNativeDisplayType _ge_openvg_disp_id = EGL_DEFAULT_DISPLAY;
mbe_t *_ge_openvg_current_canvas = NULL;

#ifndef ASSERT
#define ASSERT(x)
#endif

#define MB_2_VG_COLOR(r, g, b, a) ((((int)(0xf * r) & 0xf) << 24) |	\
				   (((int)(0xf * g) & 0xf) << 16) |	\
				   (((int)(0xf * b) & 0xf) << 16) |	\
				   ((int)(0xf * a) & 0xf))

void
mbe_set_source_rgba(mbe_t *canvas, co_comp_t r, co_comp_t g,
		    co_comp_t b, co_comp_t a) {
    VGPaint paint;
    VGuint color;

    if(paint != VG_INVALID_HANDLE && canvas->src == NULL)
	paint = canvas->paint;	/* previous one is also a color paint */
    else {
	paint = vgCreatePaint();
	ASSERT(paint != VG_INVALID_HANDLE);
	canvas->paint = paint;
	canvas->src = NULL;
    }
    
    color = MB_2_VG_COLOR(r, g, b, a);
    vgSetColor(paint, color);
    canvas->paint_installed = 0;
}

void
mbe_scissoring(mbe_t *canvas, int n_areas, area_t **areas) {
    static VGint *scissors = NULL;
    static int n_scissors = 0;
    VGint *coord;
    area_t *area;
    int i;

    _MK_CURRENT_CTX(canvas);

    if(n_areas > n_scissors) {
	if(scissors) free(scissors);
	n_scissors = (n_areas + 0xf) & 0xf;
	scissors = (VGint *)malloc(sizeof(VGint) * n_scissors * 4);
	ASSERT(scissors != NULL);
    }
    
    coord = scissors;
    for(i = 0; i < n_areas; i++) {
	area = areas[i];
	*coord++ = area->x;
	*coord++ = area->y;
	*coord++ = area->w;
	*coord++ = area->h;
    }

    vgSetiv(VG_SCISSOR_RECTS, n_areas * 4, scissors);
}

static int
_openvg_find_config(mb_img_fmt_t fmt, int w, int h,
		   EGLConfig *config) {
    EGLDisplay display;
    EGLint attrib_list[16];
    EGLConfig configs[1];
    EGLint nconfigs;
    int i = 0;
    EGLBoolean r;
    
    switch(fmt) {
    case MB_IFMT_ARGB32:
	attrib_list[i++] = EGL_RED_SIZE;
	attrib_list[i++] = 8;
	attrib_list[i++] = EGL_GREEN_SIZE;
	attrib_list[i++] = 8;
	attrib_list[i++] = EGL_BLUE_SIZE;
	attrib_list[i++] = 8;
	attrib_list[i++] = EGL_ALPHA_SIZE;
	attrib_list[i++] = 8;
	break;
	
    case MB_IFMT_RGB24:
	attrib_list[i++] = EGL_RED_SIZE;
	attrib_list[i++] = 8;
	attrib_list[i++] = EGL_GREEN_SIZE;
	attrib_list[i++] = 8;
	attrib_list[i++] = EGL_BLUE_SIZE;
	attrib_list[i++] = 8;
	break;
	
    case MB_IFMT_A8:
	attrib_list[i++] = EGL_ALPHA_SIZE;
	attrib_list[i++] = 8;
	break;
	
    case MB_IFMT_A1:
	attrib_list[i++] = EGL_ALPHA_SIZE;
	attrib_list[i++] = 1;
	break;
	
    case MB_IFMT_RGB16_565:
	attrib_list[i++] = EGL_RED_SIZE;
	attrib_list[i++] = 5;
	attrib_list[i++] = EGL_GREEN_SIZE;
	attrib_list[i++] = 6;
	attrib_list[i++] = EGL_BLUE_SIZE;
	attrib_list[i++] = 5;
	break;
	
    default:
	return -1;
    }

    attrib_list[i++] = EGL_SURFACE_TYPE;
    attrib_list[i++] = EGL_PBUFFER_BIT;
#if 0
    attrib_list[i++] = EGL_MAX_PBUFFER_WIDTH;
    attrib_list[i++] = w;
    attrib_list[i++] = EGL_MAX_PBUFFER_HEIGHT;
    attrib_list[i++] = h;
#endif
    
    attrib_list[i++] = EGL_NONE;
    
    display = _VG_DISPLAY();
    r = eglChooseConfig(display, attrib_list, configs, 1, &nconfigs);
    if(!r)
	return -1;

    *config = configs[0];
    
    return 0;
}

#ifdef EGL_GLX
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static int
_get_img_fmt_from_xvisual(Display *display, Visual *visual) {
    VisualID visual_id;
    XVisualInfo temp;
    XVisualInfo *infos;
    int n;
    int fmt = -1;
    
    visual_id = XVisualIDFromVisual(visual);
    temp.visualid = visual_id;
    infos = XGetVisualInfo(display, VisualIDMask, &temp, &n);
    if(n != 1)
	return -1;

    switch(infos->depth) {
    case 32:
	fmt = MB_IFMT_ARGB32;
	break;
	
    case 24:
	fmt = MB_IFMT_RGB24;
	break;
	
    case 16:
	fmt = MB_IFMT_RGB16_565;
	break;
	
    case 8:
	fmt = MB_IFMT_A8;
	break;
	
    case 1:
	fmt = MB_IFMT_A1;
	break;
    }

    return fmt;
}

/*! \brief Create an EGL window surface for X11.
 *
 * This function is compiled only for GLX enabled.
 */
mbe_surface_t *
mbe_vg_win_surface_create(Display *display, Drawable drawable,
			  Visual *visual, int width, int height) {
    EGLDisplay egl_disp;
    EGLSurface egl_surface;
    mbe_surface_t *surface;
    EGLConfig config;
    EGLint attrib_list[2] = {EGL_NONE};
    int fmt;
    int r;

    fmt = _get_img_fmt_from_xvisual(display, visual);
    if(fmt == -1)
	return NULL;
    
    r = _openvg_find_config(fmt, width, height, &config);
    if(r != 0)
	return NULL;

    egl_disp = eglGetDisplay(display);
    if(egl_disp == EGL_NO_DISPLAY || egl_disp != _VG_DISPLAY())
	return NULL;

    egl_surface = eglCreateWindowSurface(egl_disp, config, drawable,
					 attrib_list);

    surface = (mbe_surface_t *)malloc(sizeof(mbe_surface_t));
    if(surface == NULL) {
	eglDestroySurface(egl_disp, egl_surface);
	return NULL;
    }

    surface->surface = egl_surface;

    return surface;
}

#endif

mbe_surface_t *
mbe_image_surface_create(mb_img_fmt_t fmt, int w, int h) {
    EGLSurface surface;
    EGLDisplay display;
    EGLConfig config;
    EGLint attrib_list[5];
    mbe_surface_t *mbe_surface;
    int r;


    r = _openvg_find_config(fmt, w, h, &config);
    if(r != 0)
	return NULL;
    
    display = _VG_DISPLAY();
    attrib_list[0] = EGL_WIDTH;
    attrib_list[1] = w;
    attrib_list[2] = EGL_HEIGHT;
    attrib_list[3] = h;
    attrib_list[4] = EGL_NONE;
    /* Some implementation does not support pbuffer.
     * We need use some other surface to replace this one.
     */
    surface = eglCreatePbufferSurface(display, config, attrib_list);
    if(surface == EGL_NO_SURFACE)
	return NULL;
    
    mbe_surface = (mbe_surface_t *)malloc(sizeof(mbe_surface_t));
    if(mbe_surface == NULL) {
	eglDestroySurface(display, surface);
	return NULL;
    }
    mbe_surface->surface = surface;
    mbe_surface->asso_mbe = NULL;
    mbe_surface->w = w;
    mbe_surface->h = h;

    return mbe_surface;
}

mbe_t *
mbe_create(mbe_surface_t *surface) {
    EGLDisplay display;
    EGLConfig config;
    EGLContext ctx, shared;
    VGPath path;
    EGLint attrib_list[2] = {EGL_NONE};
    mbe_t *canvas;

    display = _VG_DISPLAY();
    ctx = eglCreateContext(display, config, shared, attrib_list);
    if(ctx == EGL_NO_CONTEXT)
	return NULL;

    path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
			1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
    if(path == VG_INVALID_HANDLE) {
	eglDestroyContext(display, ctx);
	return NULL;
    }
    
    canvas = (mbe_t *)malloc(sizeof(mbe_t));
    if(canvas == NULL) {
	eglDestroyContext(display, ctx);
	vgDestroyPath(path);
	return NULL;
    }
    
    canvas->src = NULL;
    canvas->paint = VG_INVALID_HANDLE;
    canvas->paint_installed = 0;
    canvas->tgt = surface;
    canvas->ctx = ctx;
    canvas->path = path;

    return canvas;
}

void
mbe_destroy(mbe_t *canvas) {
    EGLDisplay display;

    display = _VG_DISPLAY();
    
    vgDestroyPath(canvas->path);
    eglDestroyContext(display, canvas->ctx);
    canvas->tgt->asso_mbe = NULL; /* remove association */
    free(canvas);
}

void
mbe_paint(mbe_t *canvas) {
    EGLDisplay display;
    EGLint w, h;
    EGLBoolean r;
    VGPath path;
    
    _MK_CURRENT_CTX(canvas);
    _MK_CURRENT_PAINT(canvas);
    
    display = _VG_DISPLAY();
    
    r = eglQuerySurface(display, canvas->tgt, EGL_WIDTH, &w);
    ASSERT(r == EGL_TRUE);
    r = eglQuerySurface(display, canvas->tgt, EGL_HEIGHT, &h);
    ASSERT(r == EGL_TRUE);

    vgSeti(VG_SCISSORING, VG_FALSE);
    
    path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
			1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
    vguRect(path, 0, 0, w, h);
    vgDrawPath(path, VG_FILL_PATH);
    vgDestroyPath(path);
    
    vgSeti(VG_SCISSORING, VG_TRUE);
}

void
mbe_clear(mbe_t *canvas) {
    VGPaint paint;

    _MK_CURRENT_CTX(canvas);

    paint = vgCreatePaint();
    ASSERT(paint != VG_INVALID_HANDLE);
    vgSetColor(paint, 0);
    
    vgSetPaint(paint, VG_FILL_PATH);
    vgSeti(VG_BLEND_MODE, VG_BLEND_SRC);
    vgDrawPath(canvas->path, VG_FILL_PATH);
    vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);

    vgDestroyPaint(paint);

    canvas->paint_installed = 0;
}
