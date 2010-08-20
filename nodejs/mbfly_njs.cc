#include <stdio.h>
#include <string.h>
#include <v8.h>

extern "C" {
#include "X_supp_njs.h"
}

#include "mbfly_njs.h"

using namespace v8;

/*! \defgroup xnjsmb_mb_rt JS binding for MB runtime.
 * \ingroup xnjsmb
 *
 * @{
 */
static coord_t *
xnjsmb_coord_new(njs_runtime_t *rt, coord_t *parent, const char **err) {
    coord_t *coord;
    redraw_man_t *rdman;

    rdman = X_njs_MB_rdman(rt);
    coord = rdman_coord_new(rdman, parent);
    if(coord == NULL) {
        *err = "Can not allocate a redraw_man_t";
	return NULL;
    }

    return coord;
}

static void
xnjsmb_mb_rt_objs_mod(Handle<Object> mbrt, Handle<Value> ret) {
    Handle<Object> ret_obj = ret->ToObject();

    SET(ret_obj, "mbrt", mbrt);
}

#define xnjsmb_auto_coord_new export_xnjsmb_auto_coord_new

static void
xnjsmb_redraw_changed(njs_runtime_t *rt) {
    redraw_man_t *rdman;

    rdman = X_njs_MB_rdman(rt);
    rdman_redraw_changed(rdman);
}

static void
xnjsmb_redraw_all(njs_runtime_t *rt) {
    redraw_man_t *rdman;

    rdman = X_njs_MB_rdman(rt);
    rdman_redraw_all(rdman);
}

static njs_runtime_t *
_X_njs_MB_new(Handle<Object> self, char *display_name,
	      int width, int height) {
    njs_runtime_t *obj;
    subject_t *subject;
    Handle<Value> subject_o;

    obj = X_njs_MB_new(display_name, width, height);
    WRAP(self, obj);		/* mkroot need a wrapped object, but
				 * it is wrapped after returning of
				 * this function.  So, we wrap it
				 * here. */
    X_njs_MB_init_handle_connection(obj);
    xnjsmb_coord_mkroot(self);
    
    subject = X_njs_MB_kbevents(obj);
    subject_o = export_xnjsmb_auto_subject_new(subject);
    SET(self, "kbevents", subject_o);

    return obj;
}

/*! \defgroup njs_template_cb Callback functions for v8 engine and nodejs.
 *
 * @{
 */

/*
 * Redirect following function to respective exported version from
 * other modules.  Since gen_v8_binding.m4 make all functions static,
 * we need a exported version to call them indrectly from other
 * modules.
 */
#define xnjsmb_auto_path_new export_xnjsmb_auto_path_new
#define xnjsmb_auto_stext_new export_xnjsmb_auto_stext_new
#define xnjsmb_auto_image_new export_xnjsmb_auto_image_new
#define xnjsmb_auto_rect_new export_xnjsmb_auto_rect_new
#define xnjsmb_auto_paint_color_new export_xnjsmb_auto_paint_color_new
#define xnjsmb_auto_paint_image_new export_xnjsmb_auto_paint_image_new
#define xnjsmb_auto_paint_linear_new export_xnjsmb_auto_paint_linear_new
#define xnjsmb_auto_paint_radial_new export_xnjsmb_auto_paint_radial_new

#include "mbfly_njs-inc.h"

/* @} */

/*! \brief Get rdman associated with the runtime.
 */
redraw_man_t *
xnjsmb_rt_rdman(Handle<Object> mbrt) {
    HandleScope scope;
    njs_runtime_t *rt;
    redraw_man_t *rdman;

    rt = (njs_runtime_t *)UNWRAP(mbrt);
    rdman = X_njs_MB_rdman(rt);
    
    return rdman;
}

Handle<Value>
hello_func(const Arguments &args) {
    HandleScope scope;

    return String::Concat(String::New("World"), args[0]->ToString());
}

extern "C" void
init(Handle<Object> target) {
    HandleScope scope;
    Handle<FunctionTemplate> func, mb_rt_func;
    Handle<ObjectTemplate> rt_instance_temp, rt_proto_temp;

    func = FunctionTemplate::New(hello_func);
    target->Set(String::New("Hello"), func->GetFunction());

    /*
     * Initialize template for MadButterfly runtime objects.
     */
    xnjsmb_auto_mb_rt_init();

    /*
     * Add properties to mb_rt templates for other modules.
     */
    xnjsmb_shapes_init_mb_rt_temp(xnjsmb_auto_mb_rt_temp);
    xnjsmb_paints_init_mb_rt_temp(xnjsmb_auto_mb_rt_temp);
    xnjsmb_font_init_mb_rt_temp(xnjsmb_auto_mb_rt_temp);
    xnjsmb_img_ldr_init_mb_rt_temp(target);
    xnjsmb_observer_init();
    
    target->Set(String::New("mb_rt"),
		xnjsmb_auto_mb_rt_temp->GetFunction());    
}

/* @} */
