/*! \page ge_layer Graphic Engine Layer
 *
 * Graphic Engine Layer is an abstract of graphic engine; likes Cairo
 * and Skia.  It provides portability for the rest of MadButterfly.
 *
 * The basic stratage of interface of graphic engine layer is defined
 * according purpose of MadButterfly.  For example, MadButterfly wants
 * a function that can clear a canvas, we define a clear function.
 * Never define a indirectly way to finish the function.  Never define
 * a way to finish the function for the reason that some engine
 * require you to finish the task in that procedure.  It avoids
 * binding graphic engine layer with any behavior of a graphic engine,
 * and provides more compatible with other engines, to define
 * interface of graphic engine layer according purpose of
 * MadButterfly.
 *
 * \section ge_mem Graphic Engine Layer Memory Management
 *
 * MadButterfly is responsible for management of objects and memory
 * blocks returned by graphic engine layer, even for graphic engines
 * that have management model.  MadButterfly supposes memory blocks
 * only be released when they are no more used.  MadBufferfly is
 * responsible for release them.  So, even a graphic engine has
 * reference count with objects, MadButterfly still keep a reference
 * for every object returned by the engine until no one will use it.
 *
 * \section ge_transform Transformation of Coordination System
 *
 * Points of pathes are transformed when it is added to the canvas
 * with the transformation matrix at the time.  So, changes of
 * transformation matrix of an canvas will not affect points that had
 * been added.  It only affects points been added when the matrix is
 * setted.
 */
#include <stdio.h>
#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkRegion.h>
#include <SkShader.h>
#include <SkDevice.h>
#include <SkGradientShader.h>
#include <SkXfermode.h>
#include <SkColorFilter.h>

#define C_START extern "C" {
#define C_END }

C_START

#include "mb_graph_engine_skia.h"
#include "mb_shapes.h"
#include "mb_img_ldr.h"

/*! \brief Source pattern
 *
 * For Skia, source pattern is SkShader with some decoration.  Since
 * SkShade will repeative tiling or extenting edge color, it can not
 * stop tiling and extenting for fixed size bitmap.  So, we need to
 * translate mbe_paint() into a drawing of a rectangle.
 */
struct _mbe_pattern_t {
    SkShader *shader;
    int w, h;
    int has_size;
    co_aix matrix[6];
};

struct _mbe_scaled_font_t {
    struct _mb_font_face_t *face;
    co_aix fnt_mtx[6];
    co_aix ctm[6];
};
struct _mbe_font_face_t {};
/*! \brief MadButterfly Graphic Engine Context.
 *
 * A context comprises source pattern, target surface, path,
 * line-width, and transform matrix.
 */
struct _mbe_t {
    SkCanvas *canvas;
    SkPath *path, *subpath;
    SkPaint *paint;
    SkRegion *saved_region;
    
    struct _mbe_states_t *states;
};

struct _mbe_states_t {
    mbe_pattern_t *ptn;
    int ptn_owned;
    co_aix line_width;
    co_aix matrix[6];
    struct _mbe_states_t *next;
};

#ifndef ASSERT
#define ASSERT(x)
#endif

#define PI 3.1415926535897931

#define CO_AIX_2_SKSCALAR(a) ((SkScalar)a)
#define SKSCALAR_2_CO_AIX(a) ((co_aix)(a))
#define MB_MATRIX_2_SKMATRIX(sk, mb) {			\
	(sk).setScaleX(CO_AIX_2_SKSCALAR((mb)[0]));	\
	(sk).setSkewX(CO_AIX_2_SKSCALAR((mb)[1]));	\
	(sk).setTranslateX(CO_AIX_2_SKSCALAR((mb)[2]));	\
	(sk).setSkewY(CO_AIX_2_SKSCALAR((mb)[3]));	\
	(sk).setScaleY(CO_AIX_2_SKSCALAR((mb)[4]));	\
	(sk).setTranslateY(CO_AIX_2_SKSCALAR((mb)[5]));	\
	(sk).setPerspX(0);				\
	(sk).setPerspY(0);				\
	(sk).set(SkMatrix::kMPersp2, 1);		\
    }
#define SKMATRIX_2_MB_MATRIX(mb, sk) {				\
	(mb)[0] = SKSCALAR_2_CO_AIX((sk).getScaleX());		\
	(mb)[1] = SKSCALAR_2_CO_AIX((sk).getSkewX());		\
	(mb)[2] = SKSCALAR_2_CO_AIX((sk).getTranslateX());	\
	(mb)[3] = SKSCALAR_2_CO_AIX((sk).getSkewY());		\
	(mb)[4] = SKSCALAR_2_CO_AIX((sk).getScaleY());		\
	(mb)[5] = SKSCALAR_2_CO_AIX((sk).getTranslateY());	\
    }
#define MBSTOP_2_SKCOLOR(c)			\
    ((((int)((c)->a * 255)) << 24) |		\
     (((int)((c)->r * 255)) << 16) |		\
     (((int)((c)->g * 255)) << 8) |		\
     (((int)((c)->b * 255))))
#define MB_CO_COMP_2_SK(c) (((int)((c) * 255)) & 0xff)

static const co_aix id_matrix[6] = { 1, 0, 0, 0, 1, 0 };

static void
_prepare_sized_pattern(mbe_t *mbe, mbe_pattern_t *ptn) {
    SkCanvas *canvas = mbe->canvas;
    SkPath path;
    co_aix x, y;
    co_aix reverse[6];
    
    *mbe->saved_region = canvas->getTotalClip();
    
    compute_reverse(ptn->matrix, reverse);
    x = 0; y = 0;
    matrix_trans_pos(reverse, &x, &y);
    path.moveTo(CO_AIX_2_SKSCALAR(x), CO_AIX_2_SKSCALAR(y));
    x = 0; y = ptn->h;
    matrix_trans_pos(reverse, &x, &y);
    path.moveTo(CO_AIX_2_SKSCALAR(x), CO_AIX_2_SKSCALAR(y));
    x = ptn->w; y = ptn->h;
    matrix_trans_pos(reverse, &x, &y);
    path.moveTo(CO_AIX_2_SKSCALAR(x), CO_AIX_2_SKSCALAR(y));
    path.close();
    
    canvas->clipPath(path, SkRegion::kIntersect_Op);
}

static void
_finish_sized_pattern(mbe_t *mbe) {
    SkCanvas *canvas = mbe->canvas;
    
    canvas->setClipRegion(*mbe->saved_region);
}

static void
_canvas_device_region(SkCanvas *canvas, SkRegion *region) {
    SkDevice *device;
    int w, h;

    device = canvas->getDevice();
    w = device->width();
    h = device->height();
    region->setRect(0, 0, w, h);
}

static void
_update_path(mbe_t *mbe) {
    SkPath *path = mbe->path;
    SkPath *subpath = mbe->subpath;
    SkMatrix canvas_matrix;
    SkPoint point;

    MB_MATRIX_2_SKMATRIX(canvas_matrix, mbe->states->matrix);
    path->addPath(*subpath, canvas_matrix);
    
    subpath->getLastPt(&point);
    subpath->rewind();
    subpath->moveTo(point);
}

/*
 * When a function want to use the paint associated with a canvas to
 * draw, it should call _prepare_paint() can make the paint ready.
 * And, call _finish_paint() when the paint is no more used.
 */
static void
_prepare_paint(mbe_t *mbe, SkPaint::Style style) {
    SkPaint *paint = mbe->paint;
    mbe_pattern_t *ptn = mbe->states->ptn;
    SkShader *shader;
    co_aix matrix[6];
    SkMatrix skmatrix;

    paint->setStyle(style);
    
    if(ptn != NULL) {
	/* Local matrix of SkShader is a mapping from source pattern to
	 * user space.  Unlikely, for Cairo is a mapping from user space
	 * to source pattern.
	 */
	shader = ptn->shader;
	matrix_mul(mbe->states->matrix, ptn->matrix, matrix);
	MB_MATRIX_2_SKMATRIX(skmatrix, matrix);
	shader->setLocalMatrix(skmatrix);
	paint->setShader(shader);
    }

    if(style == SkPaint::kStroke_Style)
	paint->setStrokeWidth(CO_AIX_2_SKSCALAR(mbe->states->line_width));

    if(ptn != NULL && ptn->has_size)
	_prepare_sized_pattern(mbe, ptn);
}

static void
_finish_paint(mbe_t *mbe) {
    mbe_pattern_t *ptn = mbe->states->ptn;
    
    mbe->paint->reset();
    if(ptn != NULL && ptn->has_size)
	_finish_sized_pattern(mbe);
}

mbe_pattern_t *mbe_pattern_create_for_surface(mbe_surface_t *surface) {
    mbe_pattern_t *ptn;
    SkBitmap *bitmap = (SkBitmap *)surface;

    ptn = (mbe_pattern_t *)malloc(sizeof(mbe_pattern_t));
    ptn->shader = SkShader::CreateBitmapShader(*bitmap,
					       SkShader::kClamp_TileMode,
					       SkShader::kClamp_TileMode);
    if(ptn->shader == NULL) {
	free(ptn);
	return NULL;
    }
    
    ptn->has_size = 1;
    ptn->w = bitmap->width();
    ptn->h = bitmap->height();

    memcpy(ptn->matrix, id_matrix, sizeof(co_aix) * 6);
    
    return ptn;
}

mbe_pattern_t *
mbe_pattern_create_radial(co_aix cx0, co_aix cy0, co_aix radius0,
			  co_aix cx1, co_aix cy1, co_aix radius1,
			  grad_stop_t *stops, int stop_cnt) {
    mbe_pattern_t *ptn;
    SkColor *colors;
    SkScalar *poses;
    grad_stop_t *stop;
    SkPoint center;
    int i;

    ptn = (mbe_pattern_t *)malloc(sizeof(mbe_pattern_t));
    colors = new SkColor[stop_cnt];
    poses = new SkScalar[stop_cnt];
    if(ptn == NULL || colors == NULL || poses == NULL)
	goto fail;

    center.set(CO_AIX_2_SKSCALAR(cx1), CO_AIX_2_SKSCALAR(cy1));
    
    stop = stops;
    for(i = 0; i < stop_cnt; i++) {
	colors[i] = MBSTOP_2_SKCOLOR(stop);
	poses[i] = CO_AIX_2_SKSCALAR(stop->offset);
    }

    /*
     * cx0, cy0 and radius0 is not used.  Since Skia is still not
     * support two circles radial.  And, SVG 1.2 is also not support
     * two circles.
     */
    ptn->shader =
	SkGradientShader::CreateRadial(center, CO_AIX_2_SKSCALAR(radius1),
				       colors, poses, stop_cnt,
				       SkShader::kClamp_TileMode);
    if(ptn->shader == NULL)
	goto fail;

    memcpy(ptn->matrix, id_matrix, sizeof(co_aix) * 6);
    
    delete colors;
    delete poses;
    return ptn;
    
 fail:
    if(ptn) free(ptn);
    if(colors) delete colors;
    if(poses) delete poses;
    return NULL;
}

mbe_pattern_t *
mbe_pattern_create_linear(co_aix x0, co_aix y0,
			  co_aix x1, co_aix y1,
			  grad_stop_t *stops, int stop_cnt) {
    mbe_pattern_t *ptn;
    SkColor *colors;
    SkScalar *poses;
    grad_stop_t *stop;
    SkPoint points[2];
    int i;

    ptn = (mbe_pattern_t *)malloc(sizeof(mbe_pattern_t));
    colors = new SkColor[stop_cnt];
    poses = new SkScalar[stop_cnt];
    if(ptn == NULL || colors == NULL || poses == NULL)
	goto fail;

    points[0].set(CO_AIX_2_SKSCALAR(x0), CO_AIX_2_SKSCALAR(y0));
    points[1].set(CO_AIX_2_SKSCALAR(x1), CO_AIX_2_SKSCALAR(y1));
    
    stop = stops;
    for(i = 0; i < stop_cnt; i++) {
	colors[i] = MBSTOP_2_SKCOLOR(stop);
	poses[i] = CO_AIX_2_SKSCALAR(stop->offset);
    }

    /*
     * cx0, cy0 and radius0 is not used.  Since Skia is still not
     * support two circles radial.  And, SVG 1.2 is also not support
     * two circles.
     */
    ptn->shader =
	SkGradientShader::CreateLinear(points, colors, poses, stop_cnt,
				       SkShader::kClamp_TileMode);
    if(ptn->shader == NULL)
	goto fail;

    memcpy(ptn->matrix, id_matrix, sizeof(co_aix) * 6);
    
    delete colors;
    delete poses;
    return ptn;
    
 fail:
    if(ptn) free(ptn);
    if(colors) delete colors;
    if(poses) delete poses;
    return NULL;
}

mbe_pattern_t *
mbe_pattern_create_image(mb_img_data_t *img) {
    return NULL;
}

void mbe_pattern_set_matrix(mbe_pattern_t *ptn, const co_aix matrix[6]) {
    SkMatrix skmatrix;

    MB_MATRIX_2_SKMATRIX(skmatrix, matrix);

    ptn->shader->setLocalMatrix(skmatrix);
}

void mbe_pattern_destroy(mbe_pattern_t *ptn) {
    if(ptn->shader)
	delete ptn->shader;
    free(ptn);
}

int mbe_image_surface_get_stride(mbe_surface_t *surface) {
    return ((SkBitmap *)surface)->rowBytes();
}

int mbe_image_surface_get_height(mbe_surface_t *surface) {
    return ((SkBitmap *)surface)->height();
}

int mbe_image_surface_get_width(mbe_surface_t *surface) {
    return ((SkBitmap *)surface)->width();
}

unsigned char *mbe_image_surface_get_data(mbe_surface_t *surface) {
    return (unsigned char *)((SkBitmap *)surface)->getPixels();
}

mbe_surface_t *
mbe_image_surface_create_for_data(unsigned char *data,
				  mb_img_fmt_t fmt,
				  int width, int height,
				  int stride) {
    SkBitmap *bitmap;
    SkBitmap::Config cfg;

    switch(fmt) {
    case MB_IFMT_ARGB32:
	cfg = SkBitmap::kARGB_8888_Config; break;
	
    case MB_IFMT_A8:
	cfg = SkBitmap::kA8_Config; break;
	
    case MB_IFMT_A1:
	cfg = SkBitmap::kA1_Config; break;
	
    case MB_IFMT_RGB16_565:
	cfg = SkBitmap::kRGB_565_Config; break;
	
    case MB_IFMT_RGB24:
    default:
	return NULL;
    }
    
    bitmap = new SkBitmap();
    if(bitmap == NULL)
	return NULL;
    
    bitmap->setConfig(cfg, width, height, stride);
    bitmap->setPixels(data);

    return (mbe_surface_t *)bitmap;
}

mb_img_fmt_t mbe_image_surface_get_format(mbe_surface_t *surface) {
    SkBitmap *bitmap = (SkBitmap *)surface;
    mb_img_fmt_t fmt;
    SkBitmap::Config cfg;
    
    cfg = bitmap->getConfig();
    switch(cfg) {
    case SkBitmap::kARGB_8888_Config:
	fmt = MB_IFMT_ARGB32; break;

    case SkBitmap::kA8_Config:
	fmt = MB_IFMT_A8; break;

    case SkBitmap::kA1_Config:
	fmt = MB_IFMT_A1; break;

    case SkBitmap::kRGB_565_Config:
	fmt = MB_IFMT_RGB16_565; break;

    default:
	fmt = MB_IFMT_DUMMY;
    }

    return fmt;
}

mbe_surface_t *
mbe_image_surface_create(mb_img_fmt_t fmt, int width, int height) {
    SkBitmap *bitmap;
    SkBitmap::Config cfg;

    switch(fmt) {
    case MB_IFMT_ARGB32:
	cfg = SkBitmap::kARGB_8888_Config; break;
	
    case MB_IFMT_A8:
	cfg = SkBitmap::kA8_Config; break;
	
    case MB_IFMT_A1:
	cfg = SkBitmap::kA1_Config; break;
	
    case MB_IFMT_RGB16_565:
	cfg = SkBitmap::kRGB_565_Config; break;
	
    case MB_IFMT_RGB24:
    default:
	return NULL;
    }
    
    bitmap = new SkBitmap();
    if(bitmap == NULL)
	return NULL;
    
    bitmap->setConfig(cfg, width, height);
    bitmap->allocPixels();

    return (mbe_surface_t *)bitmap;
}

mbe_scaled_font_t *mbe_scaled_font_reference(mbe_scaled_font_t *scaled) {
}

void mbe_scaled_font_destroy(mbe_scaled_font_t *scaled) {}
mbe_font_face_t *mbe_font_face_reference(mbe_font_face_t *face) {}
mbe_scaled_font_t *
mbe_scaled_font_create(mbe_font_face_t *face, co_aix fnt_mtx[6],
		       co_aix ctm[6]) {}
mbe_scaled_font_t *mbe_get_scaled_font(mbe_t *canvas) {}
void mbe_scaled_font_text_extents(mbe_scaled_font_t *scaled,
					 const char *txt,
					 mbe_text_extents_t *extents) {}

void mbe_font_face_destroy(mbe_font_face_t *face) {}

void mbe_paint_with_alpha(mbe_t *canvas, co_aix alpha) {
    SkPaint *paint = canvas->paint;
    SkColorFilter *filter;
    SkColor color;

    color = ((uint32_t)(alpha * 255)) << 24;
    filter =
	SkColorFilter::CreatePorterDuffFilter(color,
					      SkPorterDuff::kSrcOver_Mode);
    mbe_paint(canvas);
    
}

void mbe_surface_destroy(mbe_surface_t *surface) {
    SkBitmap *bmap = (SkBitmap *)surface;
    
    delete bmap;
}

void mbe_set_source_rgba(mbe_t *canvas,
			 co_aix r, co_aix g, co_aix b, co_aix a) {
    canvas->paint->setARGB(MB_CO_COMP_2_SK(a),
			   MB_CO_COMP_2_SK(r),
			   MB_CO_COMP_2_SK(g),
			   MB_CO_COMP_2_SK(b));
    canvas->states->ptn = NULL;
}

void mbe_set_scaled_font(mbe_t *canvas,
				const mbe_scaled_font_t *scaled) {}
void mbe_set_source_rgb(mbe_t *canvas, co_aix r, co_aix g, co_aix b) {}

void mbe_set_line_width(mbe_t *canvas, co_aix width) {
    canvas->states->line_width = width;
}

mbe_font_face_t *mbe_get_font_face(mbe_t *canvas) {}

void mbe_fill_preserve(mbe_t *canvas) {
    mbe_pattern_t *ptn = canvas->states->ptn;
    SkPaint *paint = canvas->paint;
    SkPath *path = canvas->path;
    SkRegion *saved_clip = NULL;
    co_aix x, y;

    ASSERT(paint);
    ASSERT(ptn);
    ASSERT(path);

    if(!canvas->subpath->isEmpty())
	_update_path(canvas);
    
    _prepare_paint(canvas, SkPaint::kFill_Style);

    canvas->canvas->drawPath(*path, *paint);

    _finish_paint(canvas);
}

void mbe_set_source(mbe_t *canvas, mbe_pattern_t *source) {
    canvas->states->ptn = source;
}

void mbe_reset_scissoring(mbe_t *canvas) {
    SkRegion clip;

    _canvas_device_region(canvas->canvas, &clip);
    canvas->canvas->setClipRegion(clip);
}

mbe_surface_t *mbe_get_target(mbe_t *canvas) {
    return (mbe_surface_t *)&canvas->canvas->getDevice()->accessBitmap(false);
}

void mbe_close_path(mbe_t *canvas) {
    canvas->subpath->close();
}

void mbe_text_path(mbe_t *canvas, const char *txt) {}

void mbe_rectangle(mbe_t *canvas, co_aix x, co_aix y,
			  co_aix width, co_aix height) {
    SkPath *subpath = canvas->subpath;
    
    subpath->addRect(CO_AIX_2_SKSCALAR(x), CO_AIX_2_SKSCALAR(y),
		     CO_AIX_2_SKSCALAR(x + width),
		     CO_AIX_2_SKSCALAR(y + height));
}

int mbe_in_stroke(mbe_t *canvas, co_aix x, co_aix y) {
    return 0;
}

void mbe_new_path(mbe_t *canvas) {
    canvas->subpath->rewind();
    canvas->path->rewind();
}

void mbe_curve_to(mbe_t *canvas, co_aix x1, co_aix y1,
			 co_aix x2, co_aix y2,
			 co_aix x3, co_aix y3) {
    SkPath *subpath = canvas->subpath;

    subpath->cubicTo(CO_AIX_2_SKSCALAR(x1), CO_AIX_2_SKSCALAR(y1),
		     CO_AIX_2_SKSCALAR(x2), CO_AIX_2_SKSCALAR(y2),
		     CO_AIX_2_SKSCALAR(x3), CO_AIX_2_SKSCALAR(y3));
}

void mbe_restore(mbe_t *canvas) {
    struct _mbe_states_t *states;

    _update_path(canvas);
    
    states = canvas->states;
    ASSERT(states->next);
    canvas->states = states->next;
    free(states);
}

void mbe_move_to(mbe_t *canvas, co_aix x, co_aix y) {
    canvas->subpath->moveTo(CO_AIX_2_SKSCALAR(x),
			 CO_AIX_2_SKSCALAR(y));
}

void mbe_line_to(mbe_t *canvas, co_aix x, co_aix y) {
    canvas->subpath->lineTo(CO_AIX_2_SKSCALAR(x),
			    CO_AIX_2_SKSCALAR(y));
}

int mbe_in_fill(mbe_t *canvas, co_aix x, co_aix y) {
    SkRegion region, dev_region;
    bool in_fill;

    if(!canvas->subpath->isEmpty())
	_update_path(canvas);
    
    _canvas_device_region(canvas->canvas, &dev_region);
    region.setPath(*canvas->path, dev_region);
    
    in_fill = region.contains(x, y);

    return in_fill;
}

void mbe_stroke(mbe_t *canvas) {
    SkPath *path = canvas->path;
    SkPaint *paint = canvas->paint;

    ASSERT(ptn);
    ASSERT(path);
    ASSERT(paint);

    if(!canvas->subpath->isEmpty())
	_update_path(canvas);

    _prepare_paint(canvas, SkPaint::kStroke_Style);

    canvas->canvas->drawPath(*path, *paint);

    _finish_paint(canvas);

    path->rewind();
    canvas->subpath->rewind();
}

/*! \brief Create a mbe from a SkCanvas.
 *
 * It is only used for Android JNI.  It is used to create mbe_t from a
 * SkCanvas created by Canvas class of Android Java application.
 */
mbe_t *skia_mbe_create_by_canvas(SkCanvas *canvas) {
    mbe_t *mbe;
    struct _mbe_states_t *states;

    mbe = (mbe_t *)malloc(sizeof(mbe_t));
    if(mbe == NULL)
	return NULL;
    
    mbe->states = (struct _mbe_states_t *)
	malloc(sizeof(struct _mbe_states_t));
    states = mbe->states;
    if(states == NULL) {
	free(mbe);
	return NULL;
    }
    
    canvas->ref();
    mbe->canvas = canvas;
    mbe->path = new SkPath();
    mbe->subpath = new SkPath();
    mbe->saved_region = new SkRegion();
    mbe->paint = new SkPaint();
    states->ptn = NULL;
    states->ptn_owned = 0;
    states->line_width = 0;
    states->next = NULL;

    if(mbe->path == NULL ||
       mbe->subpath == NULL || mbe->paint == NULL ||
       mbe->saved_region == NULL)
	goto fail;

    memcpy(states->matrix, id_matrix, sizeof(co_aix) * 6);
    
    return mbe;

 fail:
    canvas->unref();
    if(mbe->path) delete mbe->path;
    if(mbe->subpath) delete mbe->subpath;
    if(mbe->paint) delete mbe->paint;
    if(mbe->saved_region) delete mbe->saved_region;
    free(states);
    free(mbe);
    
    return NULL;
}

mbe_t *mbe_create(mbe_surface_t *target) {
    mbe_t *mbe;
    SkBitmap *bitmap = (SkBitmap *)target;
    SkCanvas *canvas;

    canvas = new SkCanvas(*bitmap);
    if(canvas == NULL) {
	delete bitmap;
	return NULL;
    }
	
    mbe = skia_mbe_create_by_canvas(canvas);
    canvas->unref();
    
    if(mbe == NULL) {
	delete bitmap;
    }
    
    return mbe;
}

void mbe_destroy(mbe_t *canvas) {
    struct _mbe_states_t *states;
    
    canvas->canvas->unref();
    delete canvas->path;
    delete canvas->subpath;
    delete canvas->paint;
    delete canvas->saved_region;
    while(canvas->states) {
	states = canvas->states;
	canvas->states = states->next;
	
	if(states->ptn && states->ptn_owned)
	    mbe_pattern_destroy(states->ptn);
	free(states);
    }
    free(canvas);
}

void mbe_paint(mbe_t *canvas) {
    SkPaint *paint = canvas->paint;

    ASSERT(paint);
    
    _prepare_paint(canvas, SkPaint::kFill_Style);
    
    canvas->canvas->drawPaint(*paint);

    _finish_paint(canvas);
}

void mbe_save(mbe_t *canvas) {
    struct _mbe_states_t *states;

    states = (struct _mbe_states_t *)malloc(sizeof(struct _mbe_states_t));
    ASSERT(states);
    
    memcpy(states, canvas->states, sizeof(struct _mbe_states_t));
    states->next = canvas->states;
    canvas->states = states;
}

void mbe_fill(mbe_t *canvas) {
    mbe_fill_preserve(canvas);
    canvas->path->rewind();
    canvas->subpath->rewind();
}

void mbe_scissoring(mbe_t *canvas, int n_areas, area_t **areas) {
    int i;
    area_t *area;
    SkPath *path;

    mbe_new_path(canvas);
    
    path = canvas->path;
    for(i = 0; i < n_areas; i++) {
	area = areas[i];
	path->addRect(CO_AIX_2_SKSCALAR(area->x), CO_AIX_2_SKSCALAR(area->y),
		      CO_AIX_2_SKSCALAR(area->x + area->width),
		      CO_AIX_2_SKSCALAR(area->y + area->height));
    }

    canvas->canvas->clipPath(*path, SkRegion::kIntersect_Op);
    path->rewind();
}

mbe_font_face_t * mbe_query_font_face(const char *family,
					     int slant, int weight) {}
void mbe_free_font_face(mbe_font_face_t *face) {}

void mbe_clear(mbe_t *canvas) {
    SkColor color = 0;

    canvas->canvas->drawColor(color, SkPorterDuff::kClear_Mode);
}

void mbe_copy_source(mbe_t *src, mbe_t *dst) {
    SkPaint *paint = dst->paint;
    const SkBitmap *bmap;
    SkXfermode *mode;

    /* _prepare_paint(dst, SkPaint::kFill_Style); */
    mode = SkPorterDuff::CreateXfermode(SkPorterDuff::kSrc_Mode);
    paint->setXfermode(mode);
    bmap = &src->canvas->getDevice()->accessBitmap(false);

    dst->canvas->drawBitmap(*bmap, 0, 0, paint);
    
    paint->reset();
    mode->unref();
    /* _finish_paint(dst); */
}

void mbe_transform(mbe_t *mbe, co_aix matrix[6]) {
    _update_path(mbe);
    
    matrix_mul(matrix, mbe->states->matrix, mbe->states->matrix);
}

void mbe_arc(mbe_t *mbe, co_aix x, co_aix y, co_aix radius,
		    co_aix angle_start, co_aix angle_stop) {
    SkPoint point;
    SkPath *subpath = mbe->subpath;
    SkRect rect;
    SkScalar x0, y0;
    SkScalar ang_start, ang_stop;
    SkScalar sweep;
    SkScalar r;			/* radius */

    subpath->getLastPt(&point);
    x0 = point.fX;
    y0 = point.fX;
    r = CO_AIX_2_SKSCALAR(radius);
    ang_start = CO_AIX_2_SKSCALAR(angle_start * 180 / PI);
    ang_stop = CO_AIX_2_SKSCALAR(angle_stop * 180 / PI);
    
    /* Skia can only draw an arc in clockwise directly.  We negative
     * start and stop point to draw the arc in the mirror along x-axis
     * in a sub-path.  Then, the sub-path are reflected along x-axis,
     * again.  We get a right path, and add it to the path of mbe_t.
     */
    if(ang_start > ang_stop) {
	SkPath tmppath;
	SkMatrix matrix;
	co_aix reflect[6] = { 1, 0, 0,
			      0, -1, 0};
	
	rect.set(-r, -r, r, r);
	sweep = ang_start - ang_stop;
	tmppath.arcTo(rect, -ang_start, sweep, false);

	reflect[2] = x;
	reflect[5] = y;
	MB_MATRIX_2_SKMATRIX(matrix, reflect);
	subpath->addPath(tmppath, matrix);
    } else {
	rect.set(x0 - r, y0 - r, x0 + r, y0 + r);
	sweep = ang_stop - ang_start;
	subpath->arcTo(rect, ang_start, sweep, false);
    }
}


C_END
