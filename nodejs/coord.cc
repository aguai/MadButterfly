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

using namespace v8;

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

Handle<Value> export_xnjsmb_auto_coord_new(coord_t *coord) {
    xnjsmb_auto_coord_new(coord);
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
