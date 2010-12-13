#include "mb_graph_engine_openvg.h"
#include "mb_tools.h"

EGLNativeDisplayType _ge_openvg_disp_id = EGL_DEFAULT_DISPLAY;
mbe_t *_ge_openvg_current_canvas = NULL;

#ifndef ASSERT
#define ASSERT(x)
#endif

#ifndef ERR
#include <stdio.h>
#include <stdlib.h>
#define ERR(msg) do { fprintf(stderr, __FILE__ ":%d: %s", __LINE__, msg); abort(); } while(0)
#endif
#ifndef NOT_IMPLEMENT
#define NOT_IMPLEMENT(func)			\
    ERR(func " is not impmemented\n")
#endif

#define MK_ID(mtx)				\
    do {					\
	(mtx)[0] = 1;				\
	(mtx)[1] = 0;				\
	(mtx)[2] = 0;				\
	(mtx)[3] = 0;				\
	(mtx)[4] = 1;				\
	(mtx)[5] = 0;				\
	(mtx)[6] = 0;				\
	(mtx)[7] = 0;				\
	(mtx)[8] = 1;				\
    } while(0)

#define VG_MBE_SURFACE(mbe) ((mbe)->tgt->surface)

/*! \brief Information associated with VGImage.
 *
 * A VGImage can associated one of pattern or surface.  This type is
 * used to make sure previous associated pattern or surface being
 * released before new association.
 *
 * A _ge_openvg_img can be associated by mutltiple patterns and
 * surfaces.  But, at most one of associated patterns or surfaces, the
 * _ge_openvg_img can be activated for at any instant.
 * _ge_openvg_img::activated_for trace the object it being activated
 * for.  When a context will be current, the _ge_openvg_img associated
 * with its surface would be activated for the surface.  When a paint
 * wil be used, the _ge_openvg_img associated must be activated for
 * the paint.  Before activated, the old activation must be
 * deactivated.  _ge_openvg_img::deactivate_func is a pointer to
 * deactivation function of activated pattern or surface.
 *
 * \sa _ge_openvg_img_t
 * \note This is type is for internal using of OpenVG graphic engine.
 */
struct _ge_openvg_img {
    int ref;
    VGImage img;
    void *activated_for;
    void (*deactivate_func)(void *obj);
};

#define SURFACE_VG_IMG(surface) ((surface)->tgt->asso_img->img)

static EGLContext init_ctx;

/*! \brief Convert mb_img_fmt_t to VGImageFormat */
static VGImageFormat
_mb_ifmt_2_vgifmt(mb_img_fmt_t fmt) {
    VGImageFormat vgifmt;
    
    switch(fmt) {
    case MB_IFMT_ARGB32:
	vgifmt = VG_sARGB_8888;
	break;
	
    case MB_IFMT_RGB24:
	vgifmt = -1;
	break;
	
    case MB_IFMT_A8:
	vgifmt = VG_A_8;
	break;
	
    case MB_IFMT_A1:
	vgifmt = -1;
	break;
	
    case MB_IFMT_RGB16_565:
	vgifmt = VG_sRGB_565;
	break;
	
    default:
	return -1;
    }

    return vgifmt;
}

/*! \brief create image object for OpenVG */
static _ge_openvg_img_t *
_alloc_vgimage(mb_img_fmt_t fmt, int w, int h) {
    VGImage vg_img;
    VGImageFormat vgifmt;
    _ge_openvg_img_t *ge_img;
    
    vgifmt = _mb_ifmt_2_vgifmt(fmt);
    if(vgifmt == -1)
	return NULL;
    vg_img = vgCreateImage(vgifmt, w, h,
			   VG_IMAGE_QUALITY_NONANTIALIASED);
    if(vg_img == VG_INVALID_HANDLE)
	return NULL;
    ge_img = O_ALLOC(_ge_openvg_img_t);
    if(ge_img == NULL) {
	vgDestroyImage(vg_img);
	return NULL;
    }
    ge_img->ref = 1;
    ge_img->img = vg_img;
    ge_img->activated_for = NULL;
    ge_img->deactivate_func = NULL;

    return ge_img;
}

/*! \brief Free image object for OpenVG */
static void
_free_vgimage(_ge_openvg_img_t *ge_img) {
    if(--ge_img->ref > 0) {
	if(ge_img->activated_for) {
	    ge_img->deactivate_func(ge_img->activated_for);
	    ge_img->activated_for = NULL;
	}
	return;
    }
    
    vgDestroyImage(ge_img->img);
    free(ge_img);
}

static void
_ge_vg_img_deactivate_for_pattern(void *obj) {
    mbe_pattern_t *ptn = (mbe_pattern_t *)obj;
    VGPaint vg_paint;

    vg_paint = ptn->paint;
    vgPaintPattern(vg_paint, VG_INVALID_HANDLE);
}

/*! \brief Activate a VGImage for a pattern paint.
 * 
 * \sa _ge_openvg_img
 */
void
_ge_vg_img_activate_for_pattern(mbe_pattern_t *ptn) {
    _ge_openvg_img_t *ge_img;
    VGPaint vg_paint;
    VGImage vg_img;

    ge_img = ptn->asso_img;
    if(ge_img == NULL)
	return;

    if(ge_img->activated_for == (void *)ptn)
	return;

    if(ge_img->activated_for)
	ge_img->deactivate_func(ge_img->activated_for);

    ge_img->activated_for = ptn;
    ge_img->deactivate_func = _ge_vg_img_deactivate_for_pattern;
    
    vg_img = ge_img->img;
    vg_paint = ptn->paint;

    vgPaintPattern(vg_paint, vg_img);
}

/*! \brief Deactivate a VGImage for a VGSurface.
 *
 * A VGImage can not deatached from VGSurface.  But, it is not clear
 * in the document of EGL.  We assume that a VGImage can be used as
 * pattern of a paint, once associated surface is not current
 * rendering context.
 */
static void
_ge_vg_img_deactivate_for_surface(void *obj) {
    /* NOT_IMPLEMENT("_ge_vg_img_deactivate_for_surface"); */
}

/*! \brief Activate a VGImage for a surface
 * 
 * \sa _ge_openvg_img
 */
void
_ge_vg_img_activate_for_surface(mbe_surface_t *surf) {
    _ge_openvg_img_t *ge_img;
    
    ge_img = surf->asso_img;
    if(ge_img == NULL)
	return;
    
    if(ge_img->activated_for == (void *)surf)
	return;

    if(ge_img->activated_for)
	ge_img->deactivate_func(ge_img->activated_for);
    
    ge_img->activated_for = surf;
    ge_img->deactivate_func = _ge_vg_img_deactivate_for_surface;
}

/*
 * This implementation supports only from image surface.
 */
mbe_pattern_t *
mbe_pattern_create_for_surface(mbe_surface_t *surface) {
    mbe_pattern_t *pattern;
    _ge_openvg_img_t *ge_img;
    VGfloat *mtx;
    VGPaint paint;

    /* Support only from image surface */
    if(surface->asso_img == NULL)
	return NULL;

    paint = vgCreatePaint();
    if(paint == VG_INVALID_HANDLE)
	return NULL;

    ge_img = surface->asso_img;
    pattern = O_ALLOC(mbe_pattern_t);
    pattern->asso_img = ge_img;
    ge_img->ref++;		/* increase reference count */
    pattern->paint = paint;

    mtx = pattern->mtx;
    MK_ID(mtx);

    return pattern;
}

void
_mbe_load_pattern_mtx(VGfloat *mtx1, VGfloat *mtx2, int mode) {
    VGfloat affine;

    vgSeti(VG_MATRIX_MODE, mode);
    vgLoadMatrix(mtx1);
    if(mtx2)
	vgMultMatrix(mtx2);
}

static mbe_pattern_t *
_mbe_pattern_create_gradient(VGfloat *gradient, int grad_len,
			     int grad_type,
			     grad_stop_t *stops, int stop_cnt) {
    VGPaint paint;
    mbe_pattern_t *pattern;
    static VGfloat *ov_stops = 0;
    static int max_stop_cnt = 0;
    VGfloat *cur_ov_stop;
    grad_stop_t *cur_stop;
    int i;

    /* Make sure there is enough space */
    if(max_stop_cnt < stop_cnt) {
	max_stop_cnt = (stop_cnt + 0xf) & ~0xf;
	cur_ov_stop = (VGfloat *)realloc(ov_stops,
					 max_stop_cnt * sizeof(VGfloat) * 5);
	if(cur_ov_stop == NULL) {
	    max_stop_cnt = 0;
	    return NULL;
	}
	ov_stops = cur_ov_stop;
    }
    cur_ov_stop = ov_stops;

    cur_stop = stops;
    for(i = 0; i < stop_cnt; i++) {
	*cur_ov_stop++ = cur_stop->offset;
	*cur_ov_stop++ = cur_stop->r;
	*cur_ov_stop++ = cur_stop->g;
	*cur_ov_stop++ = cur_stop->b;
	*cur_ov_stop++ = cur_stop->a;
	cur_stop++;
    }
    
    paint = vgCreatePaint();
    if(paint == VG_INVALID_HANDLE)
	return NULL;
    vgSetParameteri(paint, VG_PAINT_TYPE, grad_type);
    vgSetParameterfv(paint, VG_PAINT_RADIAL_GRADIENT, grad_len, gradient);
    vgSetParameterfv(paint, VG_PAINT_COLOR_RAMP_STOPS, 5 * stop_cnt, ov_stops);

    pattern = O_ALLOC(mbe_pattern_t);
    if(pattern == NULL) {
	vgDestroyPaint(paint);
	return NULL;
    }

    pattern->paint = paint;
    pattern->asso_img = NULL;

    MK_ID(pattern->mtx);
    
    return pattern;
}

/*
 * \note OpenVG does not support start circle, it supports only focal
 * point.  It means radius0 is not working.
 */
mbe_pattern_t *
mbe_pattern_create_radial(co_aix cx0, co_aix cy0,
			  co_aix radius0,	
			  co_aix cx1, co_aix cy1,
			  co_aix radius1, grad_stop_t *stops,
			  int stop_cnt) {
    mbe_pattern_t *pattern;
    VGfloat gradient[] = {cx0, cy0, cx1, cy1, radius1};

    pattern = _mbe_pattern_create_gradient(gradient, 5,
					   VG_PAINT_TYPE_RADIAL_GRADIENT,
					   stops, stop_cnt);
    return pattern;
}

mbe_pattern_t *
mbe_pattern_create_linear(co_aix x0, co_aix y0,
			  co_aix x1, co_aix y1,
			  grad_stop_t *stops, int stop_cnt) {
    mbe_pattern_t *pattern;
    VGfloat gradient[] = {x0, y0, x1, y1};

    pattern = _mbe_pattern_create_gradient(gradient, 4,
					   VG_PAINT_TYPE_LINEAR_GRADIENT,
					   stops, stop_cnt);
    return pattern;
}

mbe_pattern_t *
mbe_pattern_create_image(mb_img_data_t *img) {
    VGPaint paint;
    mbe_pattern_t *pattern;
    _ge_openvg_img_t *ge_img;
    VGImage vg_img;
    VGImageFormat fmt = VG_sARGB_8888;
    
    /* \note OpenVG implementation supports one \ref MB_IFMT_ARGB32
     * image.
     */
    if(img->fmt != MB_IFMT_ARGB32)
	return NULL;
    
    /* Allocate objects */
    ge_img = _alloc_vgimage(MB_IFMT_ARGB32, img->w, img->h);
    pattern = O_ALLOC(mbe_pattern_t);
    paint = vgCreatePaint();
    if(ge_img == NULL || pattern == NULL || paint == VG_INVALID_HANDLE)
	goto err;
    
    /* Create and copy pixels into VGImage */
    vg_img = ge_img->img;
    vgImageSubData(vg_img, img->content, img->stride, fmt,
		   0, 0, img->w, img->h);
    
    pattern->paint = paint;
    pattern->asso_img = ge_img;

    return pattern;

 err:
    if(ge_img) _free_vgimage(ge_img);
    if(pattern) free(pattern);
    if(paint != VG_INVALID_HANDLE) vgDestroyPaint(paint);
    vgDestroyImage(vg_img);
    return NULL;
}

void
mbe_pattern_destroy(mbe_pattern_t *ptn) {
    if(ptn->asso_img)
	_free_vgimage(ptn->asso_img);
    vgDestroyPaint(ptn->paint);
    free(ptn);
}

void
mbe_pattern_set_matrix(mbe_pattern_t *ptn, co_aix *mtx) {
    co_aix rev[6];

    compute_reverse(mtx, rev);
    ptn->mtx[0] = rev[0];
    ptn->mtx[1] = rev[3];
    ptn->mtx[2] = 0;
    ptn->mtx[3] = rev[1];
    ptn->mtx[4] = rev[4];
    ptn->mtx[5] = 0;
    ptn->mtx[6] = rev[2];
    ptn->mtx[7] = rev[5];
    ptn->mtx[8] = 1;
}

void
mbe_set_source_rgba(mbe_t *canvas, co_comp_t r, co_comp_t g,
		    co_comp_t b, co_comp_t a) {
    VGPaint paint;
    VGuint color;
    VGfloat rgba[4];

    paint = canvas->paint;
    if(paint == VG_INVALID_HANDLE || canvas->src != NULL) {
	/* previous one is not a color paint */
	if(canvas->src) {
	    mbe_pattern_destroy(canvas->src);
	    canvas->src = NULL;
	}
	
	paint = vgCreatePaint();
	ASSERT(paint != VG_INVALID_HANDLE);
	vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	canvas->paint = paint;
    }
    
    rgba[0] = r;
    rgba[1] = g;
    rgba[2] = b;
    rgba[3] = a;
    vgSetParameterfv(paint, VG_PAINT_COLOR, 4, rgba);
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
	n_scissors = (n_areas + 0xf) & ~0xf;
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

    vgSeti(VG_SCISSORING, VG_TRUE);
    vgSetiv(VG_SCISSOR_RECTS, n_areas * 4, scissors);
}

static int
_openvg_find_config(mb_img_fmt_t fmt, int w, int h,
		   EGLConfig *config) {
    EGLDisplay display;
    EGLint attrib_list[32];
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

    attrib_list[i++] = EGL_RENDERABLE_TYPE;
    attrib_list[i++] = EGL_OPENVG_BIT;
    
    attrib_list[i++] = EGL_NONE;
    
    display = _VG_DISPLAY();
    r = eglChooseConfig(display, attrib_list, configs, 1, &nconfigs);
    if(!r)
	return -1;

    *config = configs[0];
    
    return 0;
}

#ifdef EGL_GLX
/*! \brief Create an EGL window surface for X11.
 *
 * This function is compiled only for GLX enabled.
 */
mbe_surface_t *
mbe_win_surface_create(void *display, void *drawable,
		       int fmt, int width, int height) {
    EGLDisplay egl_disp;
    EGLSurface egl_surface;
    mbe_surface_t *surface;
    EGLConfig config;
    EGLint attrib_list[2] = {EGL_NONE};
    int r;

    r = _openvg_find_config(fmt, width, height, &config);
    if(r != 0)
	return NULL;

    egl_disp = eglGetDisplay((Display *)display);
    if(egl_disp == EGL_NO_DISPLAY || egl_disp != _VG_DISPLAY())
	return NULL;

    egl_surface = eglCreateWindowSurface(egl_disp, config,
					 (Drawable)drawable,
					 attrib_list);

    surface = O_ALLOC(mbe_surface_t);
    if(surface == NULL) {
	eglDestroySurface(egl_disp, egl_surface);
	return NULL;
    }

    surface->surface = egl_surface;
    surface->asso_mbe = NULL;
    surface->asso_img = NULL;
    surface->fmt = fmt;

    return surface;
}

#endif

mbe_surface_t *
mbe_image_surface_create(mb_img_fmt_t fmt, int w, int h) {
    EGLSurface surface;
    EGLDisplay display;
    EGLConfig config;
    EGLint attrib_list[1] = {EGL_NONE};
    _ge_openvg_img_t *ge_img;
    mbe_surface_t *mbe_surface;
    int r;


    r = _openvg_find_config(fmt, w, h, &config);
    if(r != 0)
	return NULL;

    ge_img = _alloc_vgimage(fmt, w, h);
    if(ge_img == NULL)
	return NULL;

    display = _VG_DISPLAY();
    /* Some implementation does not support pbuffer.
     * We need use some other surface to replace this one.
     *
     * EGL does not support any attributes for pbuffer used by OpenVG.
     */
    surface = eglCreatePbufferFromClientBuffer(display, EGL_OPENVG_IMAGE,
					       (EGLClientBuffer)ge_img->img,
					       config, attrib_list);
    if(surface == EGL_NO_SURFACE) {
	_free_vgimage(ge_img);
	return NULL;
    }
    
    mbe_surface = O_ALLOC(mbe_surface_t);
    if(mbe_surface == NULL) {
	_free_vgimage(ge_img);
	eglDestroySurface(display, surface);
	return NULL;
    }
    mbe_surface->surface = surface;
    mbe_surface->asso_mbe = NULL;
    mbe_surface->asso_img = ge_img;
    mbe_surface->w = w;
    mbe_surface->h = h;
    mbe_surface->fmt = fmt;

    return mbe_surface;
}

mbe_surface_t *
mbe_image_surface_create_for_data(unsigned char *data,
				  mb_img_fmt_t fmt,
				  int width, int height,
				  int stride) {
    NOT_IMPLEMENT("mbe_image_surface_create_for_data");
    return NULL;
}

void
mbe_surface_destroy(mbe_surface_t *surface) {
    EGLDisplay display;

    display = _VG_DISPLAY();
    eglDestroySurface(display, surface->surface);
    
    if(surface->asso_mbe)
	surface->asso_mbe->tgt = NULL;

    if(surface->asso_img)
	_free_vgimage(surface->asso_img);
    
    free(surface);
}

void
mbe_copy_source(mbe_t *src_canvas, mbe_t *dst_canvas) {
    VGImage vg_img;
    EGLDisplay display;
    
    ASSERT(src_canvas->tgt->asso_img != NULL);
    
    _MK_CURRENT_CTX(dst_canvas);
    
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
    vgLoadIdentity();
    
    vg_img = SURFACE_VG_IMG(src_canvas);
    vgDrawImage(vg_img);

    display = _VG_DISPLAY();
    eglSwapBuffers(display, VG_MBE_SURFACE(dst_canvas));
}

void
mbe_flush(mbe_t *canvas) {
    EGLDisplay display;
    mbe_surface_t *surface;

    _MK_CURRENT_CTX(canvas);
    display = _VG_DISPLAY();
    surface = VG_MBE_SURFACE(canvas);
    eglSwapBuffers(display, surface);
}

mbe_t *
mbe_create(mbe_surface_t *surface) {
    EGLDisplay display;
    EGLConfig config;
    EGLContext ctx, shared;
    VGPath path;
    EGLint attrib_list[2] = {EGL_NONE};
    static VGfloat clear_color[4] = {0, 0, 0, 1};
    mbe_t *canvas;
    int r;

    display = _VG_DISPLAY();
    
    r = _openvg_find_config(surface->fmt, surface->w, surface->h, &config);
    if(r != 0)
	return NULL;
    
    /* shared = EGL_NO_CONTEXT; */
    shared = init_ctx;
    ctx = eglCreateContext(display, config, shared, attrib_list);
    if(ctx == EGL_NO_CONTEXT)
	return NULL;

    path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
			1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
    if(path == VG_INVALID_HANDLE) {
	eglDestroyContext(display, ctx);
	return NULL;
    }
    
    canvas = O_ALLOC(mbe_t);
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

    surface->asso_mbe = canvas;
    
    /* Set clear color for the context */
    _MK_CURRENT_CTX(canvas);
    vgSetfv(VG_CLEAR_COLOR, 4, clear_color);

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
mbe_paint_with_alpha(mbe_t *canvas, co_comp_t alpha) {
    VGfloat color_trans[8] = {1, 1, 1, alpha, 0, 0, 0, 0};
    EGLDisplay display;
    EGLint w, h;
    EGLBoolean r;
    
    _MK_CURRENT_CTX(canvas);

    display = _VG_DISPLAY();
    
    r = eglQuerySurface(display, canvas->tgt, EGL_WIDTH, &w);
    ASSERT(r == EGL_TRUE);
    r = eglQuerySurface(display, canvas->tgt, EGL_HEIGHT, &h);
    ASSERT(r == EGL_TRUE);

    /* Setup color transform for alpha */
#ifdef OPENVG_1_1
    vgSetfv(VG_COLOR_TRANSFORM_VALUES, 8, color_trans);
    vgSeti(VG_COLOR_TRANSFORM, VG_TRUE);
#endif
        
    mbe_paint(canvas);

#ifdef OPENVG_1_1
    vgSeti(VG_COLOR_TRANSFORM, VG_FALSE);
#endif
}

void
mbe_paint(mbe_t *canvas) {
    EGLDisplay display;
    EGLint w, h;
    EGLBoolean r;
    VGPath path;
    
    _MK_CURRENT_CTX(canvas);
    _MK_CURRENT_PAINT(canvas);
    if(canvas->src)
	_mbe_load_pattern_mtx(canvas->src->mtx, NULL,
			      VG_MATRIX_FILL_PAINT_TO_USER);
    
    display = _VG_DISPLAY();
    
    r = eglQuerySurface(display, canvas->tgt, EGL_WIDTH, &w);
    ASSERT(r == EGL_TRUE);
    r = eglQuerySurface(display, canvas->tgt, EGL_HEIGHT, &h);
    ASSERT(r == EGL_TRUE);

    /*
     * Disable scissoring and identity transform matrix.
     *
     * Transform matrix from path to surface is assigned by
     * mbe_transform().  Here, we temporary set it to identity, and
     * restore it after paint.
     */
    vgSeti(VG_SCISSORING, VG_FALSE);
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgLoadIdentity();
    
    path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
			1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
    vguRect(path, 0, 0, w, h);
    vgDrawPath(path, VG_FILL_PATH);
    vgDestroyPath(path);
    
    /* re-enable scissoring and restore transform matrix */
    vgLoadMatrix(canvas->mtx);
    vgSeti(VG_SCISSORING, VG_TRUE);
}

void
mbe_clear(mbe_t *canvas) {
    EGLDisplay display;
    EGLSurface *tgt_surface;
    EGLint w, h;
    EGLBoolean r;

    _MK_CURRENT_CTX(canvas);

    display = _VG_DISPLAY();
    
    tgt_surface = (EGLSurface *)canvas->tgt->surface;
    r = eglQuerySurface(display, tgt_surface, EGL_WIDTH, &w);
    ASSERT(r == EGL_TRUE);
    r = eglQuerySurface(display, tgt_surface, EGL_HEIGHT, &h);
    ASSERT(r == EGL_TRUE);
    
    /* Clear regions to the color specified by mbe_create() */
    vgClear(0, 0, w, h);
}

void mbe_init() {
    static EGLSurface init_surf;
    EGLDisplay display;
    EGLConfig config;
    EGLint nconfigs;
    EGLint attrib_list[] = {
	EGL_RED_SIZE, 1,
	EGL_GREEN_SIZE, 1,
	EGL_BLUE_SIZE, 1,
	EGL_RENDERABLE_TYPE, EGL_OPENVG_BIT,
	EGL_NONE};
    EGLint surf_attribs[] = {
	EGL_WIDTH, 1,
	EGL_HEIGHT, 1,
	EGL_NONE};
    EGLBoolean r;

    display = _VG_DISPLAY();
    eglInitialize(display, NULL, NULL);

    r = eglChooseConfig(display, attrib_list, &config, 1, &nconfigs);
    ASSERT(r);
    
    eglBindAPI(EGL_OPENVG_API);

    init_ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
    ASSERT(init_ctx != EGL_NO_CONTEXT);

    init_surf = eglCreatePbufferSurface(display, config, surf_attribs);
    ASSERT(init_surf != EGL_NO_SURFACE);
    
    eglMakeCurrent(display, init_surf, init_surf, init_ctx);
}

