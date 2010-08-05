#include <stdio.h>
#include <string.h>
#include <v8.h>

extern "C" {
#include "X_supp_njs.h"
}

#include "mbfly_njs.h"

using namespace v8;

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
xnjsmb_coord_mod(Handle<Object> mbrt, Handle<Value> ret) {
    Handle<Object> ret_obj = ret->ToObject();
    Persistent<Object> *ret_obj_hdl;
    coord_t *coord;

    SET(ret_obj, "mbrt", mbrt);
    coord = (coord_t *)UNWRAP(ret_obj);
    /* Keep associated js object in property store for retrieving,
     * later, without create new js object.
     */
    ret_obj_hdl = new Persistent<Object>(ret_obj);
    mb_prop_set(&coord->obj.props, PROP_JSOBJ, ret_obj_hdl);
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

    obj = X_njs_MB_new(display_name, width, height);
    WRAP(self, obj);		/* mkroot need a wrapped object, but
				 * it is wrapped after returning of
				 * this function.  So, we wrap it
				 * here. */
    X_njs_MB_init_handle_connection(obj);
    xnjsmb_coord_mkroot(self);

    return obj;
}

/*! \defgroup njs_template_cb Callback functions for v8 engine and nodejs.
 *
 * @{
 */

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
    
    target->Set(String::New("mb_rt"),
		xnjsmb_auto_mb_rt_temp->GetFunction());    
}
