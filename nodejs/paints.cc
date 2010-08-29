#include <v8.h>

extern "C" {
#include "mb.h"
}

#include "mbfly_njs.h"

using namespace v8;

#ifndef ASSERT
#define ASSERT(x)
#endif

/*! \defgroup xnjsmb_paints JS binding for paints
 * \ingroup xnjsmb
 *
 * @{
 */

/*! \page paint_gc How to manage life-cycle of paints?
 *
 * A paint is used to fill and stroke shapes.  It should keep live
 * before all shapes being stroked/or filled by it are dead or
 * filled/stroked by other paints.  So, every shape that uses a paint
 * should keep a reference to the paint.  The reference are removed
 * when a shape is filled/or stroked by other paints.
 *
 * A paint should keep a weak reference to itself.  It is used to free
 * resource before it being collected.
 */
static void
xnjsmb_paint_recycle(Persistent<Value> obj, void *parameter) {
    Persistent<Object> *paint_hdl = (Persistent<Object> *)parameter;
    paint_t *paint;
    Handle<Object> rt;
    redraw_man_t *rdman;
    int r;
    
    paint = (paint_t *)UNWRAP(*paint_hdl);
    rt = GET(*paint_hdl, "mbrt")->ToObject();
    rdman = xnjsmb_rt_rdman(rt);
    
    r = rdman_paint_free(rdman, paint);
    ASSERT(r == 0);
    
    paint_hdl->Dispose();
    delete paint_hdl;
}

static void
xnjsmb_paint_mod(Handle<Object> self, void *paint) {
    Persistent<Object> *paint_hdl;
    
    paint_hdl = new Persistent<Object>();
    *paint_hdl = Persistent<Object>::New(self);

    paint_hdl->MakeWeak(paint_hdl, xnjsmb_paint_recycle);
}

static void
xnjsmb_paint_fill(paint_t *paint, Handle<Object> self, shape_t *sh) {
    Handle<Value> rt_v;
    Handle<Object> rt_o;
    Handle<Object> sh_o;
    redraw_man_t *rdman;

    rt_v = GET(self, "mbrt");
    rt_o = rt_v->ToObject();
    rdman = xnjsmb_rt_rdman(rt_o);
    
    rdman_paint_fill(rdman, paint, sh);
    
    if(sh_get_coord(sh))
	rdman_shape_changed(rdman, sh);

    sh_o = *(Persistent<Object> *)mb_prop_get(&sh->obj.props, PROP_JSOBJ);
    SET(sh_o, "_fill_by", self);
}

static void
xnjsmb_paint_stroke(paint_t *paint, Handle<Object> self, shape_t *sh) {
    Handle<Value> rt_v;
    Handle<Object> rt_o;
    Handle<Object> sh_o;
    redraw_man_t *rdman;

    rt_v = GET(self, "mbrt");
    rt_o = rt_v->ToObject();
    rdman = xnjsmb_rt_rdman(rt_o);
    
    rdman_paint_stroke(rdman, paint, sh);
    
    if(sh_get_coord(sh))
	rdman_shape_changed(rdman, sh);

    sh_o = *(Persistent<Object> *)mb_prop_get(&sh->obj.props, PROP_JSOBJ);
    SET(sh_o, "_stroke_by", self);
}

static void
xnjsmb_paint_color_set_color(paint_t *paint, Handle<Object> self,
			     float r, float g, float b, float a) {
    Handle<Value> rt_v;
    Handle<Object> rt_o;
    redraw_man_t *rdman;

    rt_v = GET(self, "mbrt");
    rt_o = rt_v->ToObject();
    rdman = xnjsmb_rt_rdman(rt_o);

    paint_color_set(paint, r, g, b, a);

    rdman_paint_changed(rdman, paint);
}

/*! \brief Set stops for linear paint for Javascript code.
 */
static void
xnjsmb_paint_linear_set_stops(paint_t *paint, Handle<Value> stops) {
    Array *stops_o;
    Array *stop_o;
    int nstops;
    grad_stop_t *grad_stops, *old_grad_stops;
    int i;

    stops_o = Array::Cast(*stops);
    nstops = stops_o->Length();
    grad_stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * nstops);
    ASSERT(grad_stops != NULL);

    for(i = 0; i < nstops; i++) {
	stop_o = Array::Cast(*stops_o->Get(i));
	ASSERT(stop_o->Length() == 5);
	grad_stop_init(grad_stops + i,
		       stop_o->Get(0)->ToNumber()->Value(),  /* off */
		       stop_o->Get(1)->ToNumber()->Value(),  /* r */
		       stop_o->Get(2)->ToNumber()->Value(),  /* g */
		       stop_o->Get(3)->ToNumber()->Value(),  /* b */
		       stop_o->Get(4)->ToNumber()->Value()); /* a */
    }
    
    old_grad_stops = paint_linear_stops(paint, nstops, grad_stops);
    if(old_grad_stops)
	free(old_grad_stops);	/* The stops, here, were allocated for
				 * previous calling of this
				 * function. */
}

/*! \brief Set stops for radial paint for Javascript code.
 */
static void
xnjsmb_paint_radial_set_stops(paint_t *paint, Handle<Value> stops) {
    Array *stops_o;
    Array *stop_o;
    int nstops;
    grad_stop_t *grad_stops, *old_grad_stops;
    int i;

    stops_o = Array::Cast(*stops);
    nstops = stops_o->Length();
    grad_stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * nstops);
    ASSERT(grad_stops != NULL);

    for(i = 0; i < nstops; i++) {
	stop_o = Array::Cast(*stops_o->Get(i));
	ASSERT(stop_o->Length() == 5);
	grad_stop_init(grad_stops + i,
		       stop_o->Get(0)->ToNumber()->Value(),  /* off */
		       stop_o->Get(1)->ToNumber()->Value(),  /* r */
		       stop_o->Get(2)->ToNumber()->Value(),  /* g */
		       stop_o->Get(3)->ToNumber()->Value(),  /* b */
		       stop_o->Get(4)->ToNumber()->Value()); /* a */
    }
    
    old_grad_stops = paint_radial_stops(paint, nstops, grad_stops);
    if(old_grad_stops)
	free(old_grad_stops);	/* The stops, here, were allocated for
				 * previous calling of this
				 * function. */
}

#include "paints-inc.h"

/*! \defgroup xnjsmb_paints_cons Constructor of paints
 *
 * @{
 */
paint_t *
xnjsmb_paint_color_new(njs_runtime_t *rt,
		       float r, float g, float b, float a,
		       const char **err) {
    paint_t *paint;
    redraw_man_t *rdman;

    rdman = X_njs_MB_rdman(rt);
    paint = rdman_paint_color_new(rdman, r, g, b, a);
    if(paint == NULL) {
	*err = "can not allocate a paint_color_t";
	return NULL;
    }

    return paint;
}

paint_t *
xnjsmb_paint_image_new(njs_runtime_t *rt, mb_img_data_t *img,
		       const char **err) {
    paint_t *paint;
    redraw_man_t *rdman;

    rdman = X_njs_MB_rdman(rt);
    paint = rdman_paint_image_new(rdman, img);
    if(paint == NULL) {
	*err = "can not allocate a paint_image_t";
	return NULL;
    }

    return paint;
}

paint_t *
xnjsmb_paint_linear_new(njs_runtime_t *rt,
			float x1, float y1, float x2, float y2,
			const char **err) {
    paint_t *paint;
    redraw_man_t *rdman;

    rdman = X_njs_MB_rdman(rt);
    paint = rdman_paint_linear_new(rdman, x1, y1, x2, y2);
    if(paint == NULL) {
	*err = "can not allocate a paint_linear_t";
	return NULL;
    }

    return paint;
}

paint_t *
xnjsmb_paint_radial_new(njs_runtime_t *rt,
			float cx, float cy, float r,
			const char **err) {
    paint_t *paint;
    redraw_man_t *rdman;

    rdman = X_njs_MB_rdman(rt);
    paint = rdman_paint_radial_new(rdman, cx, cy, r);
    if(paint == NULL) {
	*err = "can not allocate a paint_radial_t";
	return NULL;
    }

    return paint;
}

/* @} */

/*! \defgroup xnjsmb_paints_export Exported wrapper maker for paints
 *
 * These functions are used by MB runtime to wrap C paints to JS
 * objects.
 *
 * @{
 */
Handle<Value>
export_xnjsmb_auto_paint_color_new(paint_t *paint) {
    Handle<Value> ret;
    
    ret = xnjsmb_auto_paint_color_new(paint);
    
    return ret;
}

Handle<Value>
export_xnjsmb_auto_paint_image_new(paint_t *paint) {
    Handle<Value> ret;
    
    ret = xnjsmb_auto_paint_image_new(paint);
    
    return ret;
}

Handle<Value>
export_xnjsmb_auto_paint_linear_new(paint_t *paint) {
    Handle<Value> ret;
    
    ret = xnjsmb_auto_paint_linear_new(paint);
    
    return ret;
}

Handle<Value>
export_xnjsmb_auto_paint_radial_new(paint_t *paint) {
    Handle<Value> ret;
    
    ret = xnjsmb_auto_paint_radial_new(paint);
    
    return ret;
}
/* @} */

/*! \brief Initialize paints for mbfly.
 *
 * This function is called by init() in mbfly_njs.cc when the module
 * being loaded.
 */
void xnjsmb_paints_init_mb_rt_temp(Handle<FunctionTemplate> rt_temp) {
    static int init_flag = 0;
    Handle<ObjectTemplate> rt_proto_temp;

    if(!init_flag) {
	xnjsmb_auto_paint_init();
	xnjsmb_auto_paint_color_init();
	xnjsmb_auto_paint_image_init();
	xnjsmb_auto_paint_linear_init();
	xnjsmb_auto_paint_radial_init();
	
	init_flag = 1;
    }
}

/* @} */
