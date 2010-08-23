#include <stdio.h>
#include <v8.h>

extern "C" {
#include "mb.h"
#include "mb_X_supp.h"
#include "mb_tools.h"
#include "X_supp_njs.h"
}

#include "mbfly_njs.h"

#ifndef ASSERT
#define ASSERT(x)
#endif

/*! \page jsgc How to Manage Life-cycle of Objects for Javascript.
 *
 * The life-cycle of MadButterfly ojects are simple.  A object is live
 * when it is created and dead when it is free.  When a coord or shape
 * is free, it is also removed from the tree.  There is not way to
 * remove a coord or a shape without freeing it.  So, if you want to
 * remove a coord or a shape object from the tree, you can only free
 * it.
 *
 * Javascript, in conventional, does not free an object.  It has GC,
 * the engine, being used, will free an object if it is no more
 * referenced.  So, we had better provide a removing function, but
 * actually free an object.  In idea situation, a new MB object would
 * be created for and attached on the JS object, when an object added
 * back to the tree.  But, it means we need to keep states of an
 * object and create a new one with the same states later.  It is
 * complicated.  So, once an object is removed, it is invalidated.
 *
 * I hope someone would implement a higher abstract layer, in JS, to
 * implement the idea model that recreate a new object when an
 * invalidated JS object being added back.
 *
 * An invalid object is the one with NULL internal field and obj.valid
 * == false.  The binding of MadButterfly hold a reference to every
 * object added to the tree of a mbrt (runtime object), and remove the
 * reference and invalidate it when it being removed.
 *
 * The binding also hold a weak reference to every object, it free the
 * object when the callback of the weak reference being called and it
 * being valid.  If the object is invalid, the callback does nothing.
 */

using namespace v8;

/*! \defgroup xnjsmb_coord JS binding for coord objects.
 * \ingroup xnjsmb
 *
 * @{
 */
static void
xnjsmb_coord_mod(Handle<Object> self, coord_t *coord) {
    Persistent<Object> *self_hdl;
    subject_t *subject;
    Handle<Value> subject_o;
    
    /* Keep associated js object in property store for retrieving,
     * later, without create new js object.
     */
    self_hdl = new Persistent<Object>(self);
    mb_prop_set(&coord->obj.props, PROP_JSOBJ, self_hdl);

    subject = coord->mouse_event;
    subject_o = export_xnjsmb_auto_subject_new(subject);
    SET(self, "mouse_event", subject_o);
}

static float
coord_get_index(coord_t *coord, Handle<Object> self, int idx,
		const char **err) {
    if(idx < 0 || idx >= 6) {
        *err = "Invalid index: out of range";
        return 0;
    }

    return coord_get_matrix(coord)[idx];
}

static float
coord_set_index(coord_t *coord, Handle<Object> self,
		int idx, float v, const char **err) {
    Handle<Object> js_rt;
    redraw_man_t *rdman;
    
    if(idx < 0 || idx >= 6) {
        *err = "Invalid index: out of range";
        return 0;
    }

    coord_get_matrix(coord)[idx] = v;
    
    js_rt = GET(self, "mbrt")->ToObject();
    rdman = xnjsmb_rt_rdman(js_rt);
    rdman_coord_changed(rdman, coord);

    return v;
}

static void
xnjsmb_coord_add_shape(coord_t *coord, Handle<Object> self,
			shape_t *shape, const char **err) {
    Handle<Object> js_rt;
    redraw_man_t *rdman;
    int r;
    
    js_rt = GET(self, "mbrt")->ToObject();
    rdman = xnjsmb_rt_rdman(js_rt);
    r = rdman_add_shape(rdman, shape, coord);
    if(r != 0)
	*err = "Unknown error";
}

#include "coord-inc.h"

/*! \brief This function used by \ref xnjsmb_mb_rt to wrap coord object.
 */
Handle<Value> export_xnjsmb_auto_coord_new(coord_t *coord) {
    return xnjsmb_auto_coord_new(coord);
}

/*! \brief Initialize Javascript object for root coord of a runtime.
 *
 * \param js_rt is the runtime object to create the root object for.
 *
 * After the function, js_rt.root is the object for root coord in
 * Javascript.
 */
void
xnjsmb_coord_mkroot(Handle<Object> js_rt) {
    redraw_man_t *rdman;
    coord_t *root;
    Handle<Object> obj;
    static int init_flag = 0;

    if(!init_flag) {
	xnjsmb_auto_coord_init();
	init_flag = 1;
    }
    
    rdman = xnjsmb_rt_rdman(js_rt);
    root = rdman_get_root(rdman);
    obj = xnjsmb_auto_coord_new(root).As<Object>();
    SET(obj, "mbrt", js_rt);

    SET(js_rt, "root", obj);
}

/* @} */
