// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __MBFLY_NJS_H_
#define __MBFLY_NJS_H_

#include <v8.h>
extern "C" {
#include <mb.h>
#include "X_supp_njs.h"
}

#define THROW(x)						\
    do {							\
	v8::Handle<v8::Value> exc;				\
	exc = v8::Exception::Error(v8::String::New(x));		\
	return v8::ThrowException(exc);				\
    } while(0)
#define THROW_noret(x)						\
    do {							\
	v8::Handle<v8::Value> exc;				\
	exc = v8::Exception::Error(v8::String::New(x));		\
	v8::ThrowException(exc);				\
	return;							\
    } while(0)
#define UNWRAP(o) v8::External::Unwrap((o)->GetInternalField(0))
#define WRAP(o, v) (o)->SetInternalField(0, v8::External::Wrap(v))
#define SET(o, n, v) (o)->Set(v8::String::New(n), v)
#define GET(o, n) (o)->Get(v8::String::New(n))

redraw_man_t *xnjsmb_rt_rdman(v8::Handle<v8::Object> mbrt);

/* From coord.cc */
void xnjsmb_coord_mkroot(v8::Handle<v8::Object> js_rt);
v8::Handle<v8::Value> export_xnjsmb_auto_coord_new(coord_t *coord);

/* From shapes.cc */
void xnjsmb_shapes_init_mb_rt_temp(v8::Handle<v8::FunctionTemplate> rt_temp);
shape_t *xnjsmb_path_new(njs_runtime_t *rt, const char *d);
shape_t *xnjsmb_stext_new(njs_runtime_t *rt, const char *txt,
			  float x, float y);
shape_t *xnjsmb_image_new(njs_runtime_t *rt, float x, float y,
			  float w, float h);
shape_t *xnjsmb_rect_new(njs_runtime_t *rt, float x, float y,
			 float w, float h,
			 float rx, float ry, const char **err);
v8::Handle<v8::Value> export_xnjsmb_auto_path_new(shape_t *sh);
v8::Handle<v8::Value> export_xnjsmb_auto_stext_new(shape_t *sh);
v8::Handle<v8::Value> export_xnjsmb_auto_image_new(shape_t *sh);
v8::Handle<v8::Value> export_xnjsmb_auto_rect_new(shape_t *sh);

/* From paints.cc */
void xnjsmb_paints_init_mb_rt_temp(v8::Handle<v8::FunctionTemplate> rt_temp);
paint_t *xnjsmb_paint_color_new(njs_runtime_t *rt,
				float r, float g, float b, float a,
				const char **err);
paint_t *xnjsmb_paint_image_new(njs_runtime_t *rt, mb_img_data_t *img,
				const char **err);
paint_t *xnjsmb_paint_linear_new(njs_runtime_t *rt,
				 float x1, float y1, float x2, float y2,
				 const char **err);
paint_t *xnjsmb_paint_radial_new(njs_runtime_t *rt,
				 float cx, float cy, float r,
				 const char **err);
v8::Handle<v8::Value> export_xnjsmb_auto_paint_color_new(paint_t *paint);
v8::Handle<v8::Value> export_xnjsmb_auto_paint_image_new(paint_t *paint);
v8::Handle<v8::Value> export_xnjsmb_auto_paint_linear_new(paint_t *sh);
v8::Handle<v8::Value> export_xnjsmb_auto_paint_radial_new(paint_t *sh);

/* From font.cc */
void xnjsmb_font_init_mb_rt_temp(v8::Handle<v8::FunctionTemplate> mb_rt_temp);

/* From image_ldr.cc */
void
xnjsmb_img_ldr_init_mb_rt_temp(v8::Handle<v8::Object> mb_rt_temp);

/* From observer.cc */
v8::Handle<v8::Value> export_xnjsmb_auto_subject_new(subject_t *subject);
void xnjsmb_observer_init(void);

#endif /* __MBFLY_NJS_H_ */
