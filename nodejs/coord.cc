// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdio.h>
#include <v8.h>

extern "C" {
#include "mb.h"
#include "mb_X_supp.h"
#include "mb_tools.h"
#include "njs_mb_supp.h"
}

#include "mbfly_njs.h"

#ifndef ASSERT
#define ASSERT(x)
#endif

#define OK 0

#define xnjsmb_auto_path_new export_xnjsmb_auto_path_new
#define xnjsmb_auto_stext_new export_xnjsmb_auto_stext_new
#define xnjsmb_auto_image_new export_xnjsmb_auto_image_new
#define xnjsmb_auto_rect_new export_xnjsmb_auto_rect_new
#define xnjsmb_auto_paint_color_new export_xnjsmb_auto_paint_color_new
#define xnjsmb_auto_paint_image_new export_xnjsmb_auto_paint_image_new
#define xnjsmb_auto_paint_linear_new export_xnjsmb_auto_paint_linear_new
#define xnjsmb_auto_paint_radial_new export_xnjsmb_auto_paint_radial_new


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
 * For coords, they are always attached to the tree when it is valid.
 * So, binding hold a persistent reference to it.  The reference is
 * purged when a coord being removed from the tree and being
 * invalidated.
 *
 * For any shape, it is not attached to the tree at begining, but is
 * attached to a tree laterly, or is collected by GC.  The binding
 * hold a weak reference for a new shape, and upgrade to a strong
 * reference when the shape being added to the tree.
 */

using namespace v8;

/*! \defgroup xnjsmb_coord JS binding for coord objects.
 * \ingroup xnjsmb
 *
 * @{
 */
/*! \brief Invalidate JS objects for coords and shapes in a subtree.
 *
 * \param self is the object of the root of subtree.
 *
 * \sa \ref jsgc
 */
static void
xnjsmb_coord_invalidate_subtree(coord_t *coord) {
    Persistent<Object> *child_hdl;
    Persistent<Object> *mem_hdl;
    coord_t *child;
    shape_t *mem;
    Handle<Value> _false = Boolean::New(0);

    /* Invalidate all coords in the subtree */
    FOR_COORDS_PREORDER(coord, child) {
	child_hdl = (Persistent<Object> *)mb_prop_get(&child->obj.props,
						      PROP_JSOBJ);
	/* There is no associated JS object.  Perhaps, it is created
	 * by xnjsmb_coord_clone_from_subtree().
	 */
	if(child_hdl == NULL) {
	    preorder_coord_skip_subtree(child);
	    continue;
	}
	
	SET(*child_hdl, "valid", _false);
	WRAP(*child_hdl, NULL);
	child_hdl->Dispose();
	delete child_hdl;

	/* Invalidate members of a coord */
	FOR_COORD_SHAPES(child, mem) {
	    mem_hdl = (Persistent<Object> *)mb_prop_get(&mem->obj.props,
							PROP_JSOBJ);
	    /* There is no associated JS object.  Perhaps, it is
	     * created by xnjsmb_coord_clone_from_subtree().
	     */
	    if(mem_hdl == NULL)
		continue;
	    
	    SET(*mem_hdl, "valid", _false);
	    WRAP(*mem_hdl, NULL);
	    mem_hdl->Dispose();
	    delete mem_hdl;
	}
    }
}

/*! \brief Free C objects for coords and shapes in a subtree.
 *
 * \param self is the object of the root of subtree.
 *
 * \sa \ref jsgc
 */
static void
xnjsmb_coord_free_subtree(redraw_man_t *rdman, coord_t *coord) {
    coord_t *child, *last_child;
    shape_t *mem, *last_mem;
    int r;

    rdman_coord_changed(rdman, coord);

    last_child = NULL;
    FOR_COORDS_POSTORDER(coord, child) {
	if(last_child != NULL) {
	    r = rdman_coord_free(rdman, last_child);
	    if(r != OK)
		THROW_noret("Unknown error");
	}

	/* Free members of a coord */
	last_mem = NULL;
	FOR_COORD_SHAPES(child, mem) {
	    if(last_mem != NULL) {
		r = rdman_shape_free(rdman, last_mem);
		if(r != OK)
		    THROW_noret("Unknown error");
	    }

	    last_mem = mem;
	}
	if(last_mem != NULL) {
	    r = rdman_shape_free(rdman, last_mem);
	    if(r != OK)
		THROW_noret("Unknown error");
	}

	last_child = child;
    }
    if(last_child != NULL) {
	r = rdman_coord_free(rdman, last_child);
	if(r != OK)
	    THROW_noret("Unknown error");
    }
}

static void
xnjsmb_coord_mod(Handle<Object> self, coord_t *coord) {
    Persistent<Object> *self_hdl;
    subject_t *subject;
    Handle<Value> subject_o;

    /* Keep associated js object in property store for retrieving,
     * later, without create new js object.
     */
    self_hdl = new Persistent<Object>();
    *self_hdl = Persistent<Object>::New(self);
    mb_prop_set(&coord->obj.props, PROP_JSOBJ, self_hdl);

    subject = coord->mouse_event;
    subject_o = export_xnjsmb_auto_subject_new(subject);
    SET(self, "mouse_event", subject_o);
    SET(self, "valid", Boolean::New(1));
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

    if (coord_get_matrix(coord)[idx] == v) return v;

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
    Persistent<Object> *shape_hdl;
    redraw_man_t *rdman;
    int r;

    js_rt = GET(self, "mbrt")->ToObject();
    rdman = xnjsmb_rt_rdman(js_rt);
    r = rdman_add_shape(rdman, shape, coord);
    if(r != 0)
	*err = "Unknown error";

    /* see \ref jsgc */
    shape_hdl = (Persistent<Object> *)mb_prop_get(&shape->obj.props,
						  PROP_JSOBJ);
    shape_hdl->ClearWeak();
    rdman_shape_changed(rdman, shape);
}

static void
xnjsmb_coord_remove(coord_t *coord, Handle<Object> self) {
    Handle<Object> js_rt;
    redraw_man_t *rdman;

    if(!GET(self, "valid")->ToBoolean()->Value()) /* Invalidated object */
	THROW_noret("Invalid object");

    js_rt = GET(self, "mbrt")->ToObject();
    rdman = xnjsmb_rt_rdman(js_rt);

    xnjsmb_coord_invalidate_subtree(coord);
    xnjsmb_coord_free_subtree(rdman, coord);
}

static void
_xnjsmb_coord_clone_from_subtree_mod(Handle<Object> src, Handle<Value> ret) {
    Handle<Object> js_rt;
    Handle<Object> ret_obj = ret->ToObject();
    coord_t *ret_coord, *child;
    Handle<Object> child_obj;

    js_rt = GET(src, "mbrt")->ToObject();
    SET(ret_obj, "mbrt", js_rt);

    /* Only root of the subtree is warpped.  Descendants of subtree
     * are not wrapped by JS object.  We have no any method to access
     * children and members of a coord, now.  So, it is fine.  But,
     * sometime later, we will provide APIs for traveling a tree.  At
     * that time, we need to create wrappers for all descendants.
     */
    ret_coord = (coord_t *)UNWRAP(ret_obj);
    xnjsmb_coord_mod(ret_obj, ret_coord);
}

static coord_t *
xnjsmb_coord_clone_from_subtree(coord_t *coord, Handle<Object> self,
				coord_t *src, const char **err) {
    Handle<Object> js_rt;
    redraw_man_t *rdman;
    coord_t *cloning;

    js_rt = GET(self, "mbrt")->ToObject();
    ASSERT(js_rt != NULL);
    rdman = xnjsmb_rt_rdman(js_rt);

    cloning = rdman_coord_clone_from_subtree(rdman, coord, src);
    if(cloning == NULL) {
	*err = "can not clone a subtree (allocate memory)";
	return NULL;
    }
    rdman_coord_changed(rdman, cloning);

    return cloning;
}

static void
xnjsmb_coord_show(coord_t *coord, Handle<Object> self) {
    Handle<Object> js_rt;
    redraw_man_t *rdman;

    js_rt = GET(self, "mbrt")->ToObject();
    ASSERT(js_rt != NULL);
    rdman = xnjsmb_rt_rdman(js_rt);
    if ((coord->flags & COF_HIDDEN) == 0) return;
    coord_show(coord);
    rdman_coord_changed(rdman, coord);
}

static void
xnjsmb_coord_hide(coord_t *coord, Handle<Object> self) {
    Handle<Object> js_rt;
    redraw_man_t *rdman;

    js_rt = GET(self, "mbrt")->ToObject();
    ASSERT(js_rt != NULL);
    rdman = xnjsmb_rt_rdman(js_rt);

    if ((coord->flags & COF_HIDDEN) != 0) return;
    coord_hide(coord);
    rdman_coord_changed(rdman, coord);
}

static void
xnjsmb_coord_set_opacity(Handle<Object> self, coord_t *coord, Handle<Value> value, const char **str)
{
    Handle<Object> js_rt;
    redraw_man_t *rdman;
    
    js_rt = GET(self, "mbrt")->ToObject();
    ASSERT(js_rt != NULL);
    rdman = xnjsmb_rt_rdman(js_rt);

    if (coord_get_opacity(coord) == value->NumberValue()) return;
    
    coord_set_opacity(coord, value->NumberValue());
    rdman_coord_changed(rdman, coord);
}

static Handle<Value>
xnjsmb_coord_get_opacity(Handle<Object> self, coord_t *coord,
			      const char **err) {
    float opacity;

    opacity = coord_get_opacity(coord);
    return Number::New(opacity);
}

#define COORD_NEXT_SIBLING(child) STAILQ_NEXT(coord_t, sibling, (child))

static mb_obj_t *
_coord_get_nth_child_member(coord_t *coord, int idx) {
    geo_t *geo;
    coord_t *child;
    int member_idx = 0;
    int cnt = 0;
    
    child = STAILQ_HEAD(coord->children);
    FOR_COORD_MEMBERS(coord, geo) {
	while(child != NULL && child->before_pmem == member_idx) {
	    if(cnt == idx)
		return (mb_obj_t *)child;
	    cnt++;
	    child = COORD_NEXT_SIBLING(child);
	}
	
	if(cnt == idx)
	    return (mb_obj_t *)geo->shape;
	cnt++;
	member_idx++;
    }
    
    while(child != NULL) {
	if(cnt == idx)
	    return (mb_obj_t *)child;
	cnt++;
	child = COORD_NEXT_SIBLING(child);
    }

    return NULL;
}

static int
xnjsmb_coord_num_children(coord_t *coord, Handle<Object> self) {
    int children_n_member_total;
    int children_cnt;
    coord_t *child;
    
    children_cnt = 0;
    FOR_COORD_CHILDREN(coord, child) {
	children_cnt++;
    }
    children_n_member_total = coord->num_members + children_cnt;

    return children_n_member_total;
}

static Handle<Value> xnjsmb_auto_coord_new(coord_t *data);

static Handle<Value>
_mb_obj_to_value(Handle<Object> parent, mb_obj_t *mbobj) {
    Handle<Value> val;
    Handle<Object> obj;
    Handle<Value> mbrt;
    Handle<Object> mbrt_o;
    Persistent<Object> *hdl;
    
    hdl = (Persistent<Object> *)mb_prop_get(&mbobj->props,
					    PROP_JSOBJ);
    if(hdl)
	return *hdl;
    
    switch(mbobj->obj_type) {
    case MBO_COORD:
	val = xnjsmb_auto_coord_new((coord_t *)mbobj);
	break;

    case MBO_PATH:
	val = xnjsmb_auto_path_new((shape_t *)mbobj);
	break;

    case MBO_RECT:
	val = xnjsmb_auto_rect_new((shape_t *)mbobj);
	break;

    case MBO_IMAGE:
	val = xnjsmb_auto_image_new((shape_t *)mbobj);
	break;

    case MBO_STEXT:
	val = xnjsmb_auto_stext_new((shape_t *)mbobj);
	break;
    }
    ASSERT(val != NULL);

    mbrt = GET(parent, "mbrt");
    obj = val->ToObject();
    mbrt_o = mbrt->ToObject();
    SET(obj, "mbrt", mbrt_o);

    return val;
}

static Handle<Value>
xnjsmb_coord_get_child(coord_t *coord, Handle<Object> self,
		       int idx, const char **err) {
    Handle<Value> child;
    mb_obj_t *child_mbobj;

    child_mbobj = _coord_get_nth_child_member(coord, idx);
    if(child_mbobj == NULL) {
	*err = "invalid index value for children";
	return Handle<Value>(NULL);
    }
    child = _mb_obj_to_value(self, child_mbobj);
    
    return child;
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
