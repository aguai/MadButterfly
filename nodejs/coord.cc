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

static Handle<Value>
xnjsmb_coord_get_index(uint32_t index, const AccessorInfo &info) {
    HandleScope scope;
    Handle<Object> self;
    coord_t *coord;
    co_aix v;

    if(index < 0 || index >= 6)
	THROW("Invalid index");
    
    self = info.This();
    coord = (coord_t *)UNWRAP(self);
    v = coord_get_matrix(coord)[index];

    return Number::New(v);
}

static Handle<Value>
xnjsmb_coord_set_index(uint32_t index, Local<Value> value,
		       const AccessorInfo &info) {
    
    HandleScope scope;
    Handle<Object> self;
    Handle<Object> js_rt;
    redraw_man_t *rdman;
    coord_t *coord;
    co_aix v;

    if(index < 0 || index >= 6)
	THROW("Invalid Index");
    if(!value->IsNumber())
	THROW("Invalid value");

    self = info.This();
    coord = (coord_t *)UNWRAP(self);
    v = value->NumberValue();
    coord_get_matrix(coord)[index] = v;

    js_rt = GET(self, "mbrt")->ToObject();
    rdman = xnjsmb_rt_rdman(js_rt);
    rdman_coord_changed(rdman, coord);

    return value;
}

/*! \brief Callback functio to add a shape to a coord in Javascript.
 *
 * coord.add_shape(shape)
 */
static Handle<Value>
xnjsmb_coord_add_shape(const Arguments &args) {
    int argc = args.Length();
    Handle<Object> self = args.This();
    Handle<Object> shape_obj;
    Handle<Object> rt_obj;
    Handle<Value> rt_val;
    redraw_man_t *rdman;
    coord_t *coord;
    shape_t *sh;
    int r;

    if(argc != 1)
	THROW("Invalid number of arguments (!= 1)");
    
    shape_obj = args[0]->ToObject();
    sh = (shape_t *)UNWRAP(shape_obj);
    ASSERT(sh != NULL);
    
    coord = (coord_t *)UNWRAP(self);
    ASSERT(coord != NULL);

    rt_val = GET(self, "mbrt");
    rt_obj = rt_val->ToObject();
    rdman = xnjsmb_rt_rdman(rt_obj);
    
    r = rdman_add_shape(rdman, sh, coord);
    if(r != 0)
	THROW("Unknown error");

    return Null();
}

static Persistent<ObjectTemplate> coord_obj_temp;

static void
xnjsmb_init_temp(void) {
    Handle<FunctionTemplate> add_shape_temp;
    
    coord_obj_temp = Persistent<ObjectTemplate>::New(ObjectTemplate::New());
    coord_obj_temp->SetIndexedPropertyHandler(xnjsmb_coord_get_index,
					      xnjsmb_coord_set_index);
    coord_obj_temp->SetInternalFieldCount(1);

    add_shape_temp = FunctionTemplate::New(xnjsmb_coord_add_shape);
    SET(coord_obj_temp, "add_shape", add_shape_temp);
}

/*! \brief Create and initialize a Javascript object for a coord.
 */
static Handle<Object>
xnjsmb_coord_new_jsobj(coord_t *coord, Handle<Object> parent_obj,
		       Handle<Object> js_rt) {
    Handle<Object> coord_obj;
    static int init_temp = 0;
    
    if(!init_temp) {
	xnjsmb_init_temp();
	init_temp = 1;
    }

    coord_obj = coord_obj_temp->NewInstance();
    ASSERT(coord_obj != NULL);
    WRAP(coord_obj, coord);

    if(!parent_obj.IsEmpty())
	SET(coord_obj, "parent", parent_obj);
    SET(coord_obj, "mbrt", js_rt);

    return coord_obj;
}

/*! \brief Create a coord object associated with the rdman of the runtime.
 *
 * Two internal fields, coord and rdman.
 */
Handle<Value>
xnjsmb_coord_new(const Arguments &args) {
    HandleScope scope;
    Handle<Object> js_rt;
    Handle<Object> coord_obj, parent_obj;
    njs_runtime_t *rt;
    redraw_man_t *rdman;
    coord_t *coord, *parent = NULL;
    int argc;

    argc = args.Length();
    if(argc > 1)
	THROW("Too many arguments (> 1)");

    js_rt = args.This();
    rt = (njs_runtime_t *)UNWRAP(js_rt);
    rdman = X_njs_MB_rdman(rt);

    if(argc == 1) {
	parent_obj = args[0]->ToObject();
	parent = (coord_t *)UNWRAP(parent_obj);
    }
    
    coord = rdman_coord_new(rdman, parent);
    ASSERT(coord != NULL);
    coord_obj = xnjsmb_coord_new_jsobj(coord, parent_obj, js_rt);

    scope.Close(coord_obj);
    
    return coord_obj;
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
    
    rdman = xnjsmb_rt_rdman(js_rt);
    root = rdman_get_root(rdman);
    obj = xnjsmb_coord_new_jsobj(root, Handle<Object>(NULL), js_rt);

    SET(js_rt, "root", obj);
}