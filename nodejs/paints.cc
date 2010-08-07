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

static void
xnjsmb_paint_fill(paint_t *paint, Handle<Object> self, shape_t *sh) {
    Handle<Value> rt_v;
    Handle<Object> rt_o;
    redraw_man_t *rdman;

    rt_v = GET(self, "mbrt");
    rt_o = rt_v->ToObject();
    rdman = xnjsmb_rt_rdman(rt_o);
    
    rdman_paint_fill(rdman, paint, sh);
    
    if(sh_get_coord(sh))
	rdman_shape_changed(rdman, sh);
}

static void
xnjsmb_paint_stroke(paint_t *paint, Handle<Object> self, shape_t *sh) {
    Handle<Value> rt_v;
    Handle<Object> rt_o;
    redraw_man_t *rdman;

    rt_v = GET(self, "mbrt");
    rt_o = rt_v->ToObject();
    rdman = xnjsmb_rt_rdman(rt_o);
    
    rdman_paint_stroke(rdman, paint, sh);
    
    if(sh_get_coord(sh))
	rdman_shape_changed(rdman, sh);
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
	
	/* xnjsmb_init_paints(); */
	init_flag = 1;
    }
    /*
    rt_proto_temp = rt_temp->PrototypeTemplate();
    SET(rt_proto_temp, "paint_color_new", xnjsmb_paint_color_new_temp);
    SET(rt_proto_temp, "paint_image_new", xnjsmb_paint_image_new_temp);
    */
}

/* @} */
