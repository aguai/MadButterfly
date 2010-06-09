#include <v8.h>
#include "mbfly_njs.h"

extern "C" {
#include <mb.h>
}

#ifndef ASSERT
#define ASSERT(x)
#endif

using namespace v8;

/*! \defgroup shape_temp Templates for shape and derivations.
 *
 * @{
 */
static Persistent<FunctionTemplate> xnjsmb_shape_temp;

static Handle<Value>
xnjsmb_shape_show(const Arguments &args) {
    shape_t *sh;
    Handle<Object> self;

    self = args.This();
    sh = (shape_t *)UNWRAP(self);
    sh_show(sh);
    
    return Null();
}

static Handle<Value>
xnjsmb_shape_hide(const Arguments &args) {
    shape_t *sh;
    Handle<Object> self;

    self = args.This();
    sh = (shape_t *)UNWRAP(self);
    sh_hide(sh);
    
    return Null();
}

/*! \brief Get stroke width of a shape.
 */
static Handle<Value>
xnjsmb_shape_stroke_width_getter(Local<String> property,
				 const AccessorInfo &info) {
    Handle<Object> self = info.This();
    shape_t *sh;
    float w;
    Handle<Value> w_val;
    
    sh = (shape_t *)UNWRAP(self);
    w = sh_get_stroke_width(sh);
    w_val = Number::New(w);
    
    return w_val;
}

/*! \brief Set stroke width of a shape.
 */
static void
xnjsmb_shape_stroke_width_setter(Local<String> property,
				 Local<Value> value,
				 const AccessorInfo &info) {
    Handle<Object> self = info.This();
    Handle<Object> rt;
    shape_t *sh;
    redraw_man_t *rdman;
    float w;
    Handle<Number> w_num;
    
    sh = (shape_t *)UNWRAP(self);
    w_num = value->ToNumber();
    w = w_num->Value();

    sh_set_stroke_width(sh, w);

    /*
     * Mark changed.
     */
    rt = GET(self, "mbrt")->ToObject();
    ASSERT(rt != NULL);
    rdman = xnjsmb_rt_rdman(rt);
    
    rdman_shape_changed(rdman, sh);
}

static void
xnjsmb_init_shape_temp(void) {
    Handle<FunctionTemplate> temp;
    Handle<ObjectTemplate> proto_temp;
    Handle<FunctionTemplate> method_temp;

    temp = FunctionTemplate::New();
    temp->SetClassName(String::New("shape"));
    xnjsmb_shape_temp = Persistent<FunctionTemplate>::New(temp);
    proto_temp = temp->PrototypeTemplate();
    
    method_temp = FunctionTemplate::New(xnjsmb_shape_show);
    SET(proto_temp, "show", method_temp);
    method_temp = FunctionTemplate::New(xnjsmb_shape_hide);
    SET(proto_temp, "hide", method_temp);

    proto_temp->SetAccessor(String::New("stroke_width"),
			    xnjsmb_shape_stroke_width_getter,
			    xnjsmb_shape_stroke_width_setter);
}

/* @} */

/*! \defgroup path_temp Templates for path objects.
 *
 * @{
 */
static Persistent<FunctionTemplate> xnjsmb_path_temp;

/*! \brief Callback of constructor of path objects for Javascript.
 */
static Handle<Value>
xnjsmb_shape_path(const Arguments &args) {
    shape_t *sh;
    redraw_man_t *rdman;
    Handle<Object> self = args.This(); // path object
    Handle<Object> rt;
    char *dstr;
    int argc;

    argc = args.Length();
    if(argc != 2)
	THROW("Invalid number of arugments (!= 1)");
    if(!args[0]->IsString())
	THROW("Invalid argument type (should be a string)");
    if(!args[1]->IsObject())
	THROW("Invalid argument type (should be an object)");

    String::Utf8Value dutf8(args[0]->ToString());
    dstr = *dutf8;

    rt = args[1]->ToObject();
    rdman = xnjsmb_rt_rdman(rt);
    sh = rdman_shape_path_new(rdman, dstr);

    WRAP(self, sh);
    SET(self, "mbrt", rt);

    return Null();
}

/*! \brief Initial function template for constructor of path objects.
 */
static void
xnjsmb_init_path_temp(void) {
    Handle<FunctionTemplate> temp;
    Handle<ObjectTemplate> inst_temp;

    temp = FunctionTemplate::New(xnjsmb_shape_path);
    temp->Inherit(xnjsmb_shape_temp);
    temp->SetClassName(String::New("path"));

    inst_temp = temp->InstanceTemplate();
    inst_temp->SetInternalFieldCount(1);

    xnjsmb_path_temp = Persistent<FunctionTemplate>::New(temp);
}

/*! \brief Callback function of mb_rt.path_new().
 */
static Handle<Value>
xnjsmb_shape_path_new(const Arguments &args) {
    HandleScope scope;
    Handle<Object> self = args.This(); // runtime object
    Handle<Object> path_obj;
    Handle<Value> path_args[2];
    int argc;

    argc = args.Length();
    if(argc != 1)
	THROW("Invalid number of arugments (!= 1)");
    if(!args[0]->IsString())
	THROW("Invalid argument type (shoud be a string)");

    path_args[0] = args[0];
    path_args[1] = self;
    
    path_obj = xnjsmb_path_temp->GetFunction()->NewInstance(2, path_args);
    
    scope.Close(path_obj);
    return path_obj;
}

/* @} */

/*! \brief Set properties of template of mb_rt.
 */
void
xnjsmb_shapes_init_mb_rt_temp(Handle<FunctionTemplate> rt_temp) {
    Handle<FunctionTemplate> path_new_temp;
    Handle<ObjectTemplate> rt_proto_temp;
    static int temp_init_flag = 0;

    if(temp_init_flag == 0) {
	xnjsmb_init_shape_temp();
	xnjsmb_init_path_temp();
	temp_init_flag = 1;
    }

    rt_proto_temp = rt_temp->PrototypeTemplate();
    
    path_new_temp = FunctionTemplate::New(xnjsmb_shape_path_new);
    SET(rt_proto_temp, "path_new", path_new_temp);
}
