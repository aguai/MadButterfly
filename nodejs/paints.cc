#include <v8.h>

extern "C" {
#include "mb.h"
}

#include "mbfly_njs.h"

using namespace v8;

#ifndef ASSERT
#define ASSERT(x)
#endif


/*! \brief Fill a shape with the paint.
 */
static Handle<Value>
xnjsmb_paint_fill(const Arguments &args) {
    int argc = args.Length();
    Handle<Object> self = args.This();
    Handle<Object> sh_obj;
    Handle<Object> rt;
    Handle<Value> rt_val;
    paint_t *paint;
    shape_t *sh;
    redraw_man_t *rdman;

    if(argc != 1)
	THROW("Invalid number of arguments (!= 1)");
    if(!args[0]->IsObject())
	THROW("Invalid argument type (shape)");

    paint = (paint_t *)UNWRAP(self);
    
    sh_obj = args[0]->ToObject();
    sh = (shape_t *)UNWRAP(sh_obj);

    rt_val = GET(self, "mbrt");
    rt = rt_val->ToObject();
    rdman = xnjsmb_rt_rdman(rt);
    
    rdman_paint_fill(rdman, paint, sh);
    
    if(sh_get_coord(sh))
	rdman_shape_changed(rdman, sh);
    
    return Null();
}

/*! \brief Stroke a shape with the paint.
 */
static Handle<Value>
xnjsmb_paint_stroke(const Arguments &args) {
    int argc = args.Length();
    Handle<Object> self = args.This();
    Handle<Object> sh_obj;
    Handle<Object> rt;
    Handle<Value> rt_val, sh_val;
    paint_t *paint;
    shape_t *sh;
    redraw_man_t *rdman;

    if(argc != 1)
	THROW("Invalid number of arguments (!= 1)");
    if(!args[0]->IsObject())
	THROW("Invalid argument type (shape)");

    paint = (paint_t *)UNWRAP(self);
    
    sh_val = args[0];
    sh_obj = sh_val->ToObject();
    sh = (shape_t *)UNWRAP(sh_obj);

    rt_val = GET(self, "mbrt");
    rt = rt_val->ToObject();
    rdman = xnjsmb_rt_rdman(rt);
    
    rdman_paint_stroke(rdman, paint, sh);
    
    if(sh_get_coord(sh))
	rdman_shape_changed(rdman, sh);
    
    return Null();
}

/*! \brief Constructor of color paint_color_t object for Javascript.
 */
static Handle<Value>
xnjsmb_paint_color(const Arguments &args) {
    int argc = args.Length();
    Handle<Object> self = args.This();
    Handle<Object> rt;
    redraw_man_t *rdman;
    paint_t *paint;
    float r, g, b, a;

    if(argc != 5)
	THROW("Invalid number of arguments (!= 5)");
    if(!args[0]->IsObject() || !args[1]->IsNumber() ||
       !args[2]->IsNumber() || !args[3]->IsNumber() ||
       !args[4]->IsNumber())
	THROW("Invalid argument type");

    rt = args[0]->ToObject();
    r = args[1]->ToNumber()->Value();
    g = args[2]->ToNumber()->Value();
    b = args[3]->ToNumber()->Value();
    a = args[4]->ToNumber()->Value();

    rdman = xnjsmb_rt_rdman(rt);
    paint = rdman_paint_color_new(rdman, r, g, b, a);
    ASSERT(sh != NULL);

    WRAP(self, paint);
    SET(self, "mbrt", rt);

    return Null();
}

static Persistent<FunctionTemplate> xnjsmb_paint_temp;
static Persistent<FunctionTemplate> xnjsmb_paint_color_temp;

/*! \brief Create and return a paint_color object.
 */
static Handle<Value>
xnjsmb_paint_color_new(const Arguments &args) {
    HandleScope scope;
    Handle<Object> rt = args.This();
    Handle<Object> paint_color_obj;
    Handle<Function> paint_color_func;
    Handle<Value> pc_args[5];
    int argc;
    int i;

    argc = args.Length();
    if(argc != 4)
	THROW("Invalid number of arguments (r, g, b, a)");
    for(i = 0; i < 4; i++)
	if(!args[i]->IsNumber())
	    THROW("Invalid argument type");
    
    pc_args[0] = rt;
    pc_args[1] = args[0];	// r
    pc_args[2] = args[1];	// g
    pc_args[3] = args[2];	// b
    pc_args[4] = args[3];	// a
    paint_color_func = xnjsmb_paint_color_temp->GetFunction();
    paint_color_obj = paint_color_func->NewInstance(5, pc_args);

    scope.Close(paint_color_obj);
    return paint_color_obj;
}

static Persistent<FunctionTemplate> xnjsmb_paint_color_new_temp;

/*! \brief Constructor of paint_image_t objects for Javascript.
 */
static Handle<Value>
xnjsmb_paint_image(const Arguments &args) {
    int argc = args.Length();
    Handle<Object> rt;
    Handle<Object> self = args.This();
    Handle<Object> img_obj;
    redraw_man_t *rdman;
    mb_img_data_t *img;
    paint_t *paint;

    if(argc != 2)
	THROW("Invalid number of arguments (!= 2)");
    if(!args[0]->IsObject() || !args[1]->IsObject())
	THROW("Invalid argument type");
    
    rt = args[0]->ToObject();
    img_obj = args[1]->ToObject();
    
    rdman = xnjsmb_rt_rdman(rt);
    img = (mb_img_data_t *)UNWRAP(img_obj);

    paint = rdman_paint_image_new(rdman, img);
    ASSERT(paint != NULL);
    
    WRAP(self, paint);
    SET(self, "mbrt", rt);
    
    return Null();
}

static Persistent<FunctionTemplate> xnjsmb_paint_image_temp;

/*! \brief Create and return a paint_image object.
 */
static Handle<Value>
xnjsmb_paint_image_new(const Arguments &args) {
    int argc = args.Length();
    HandleScope scope;
    Handle<Object> rt = args.This();
    Handle<Object> paint_image_obj;
    Handle<Value> pi_args[2];
    Handle<Function> paint_image_func;

    if(argc != 1)
	THROW("Invalid number of arguments (!= 2)");
    if(!args[0]->IsObject())
	THROW("Invalid argument type");

    pi_args[0] = rt;
    pi_args[1] = args[0];	// image
    paint_image_func = xnjsmb_paint_image_temp->GetFunction();
    paint_image_obj = paint_image_func->NewInstance(2, pi_args);

    scope.Close(paint_image_obj);
    return paint_image_obj;
}

static Persistent<FunctionTemplate> xnjsmb_paint_image_new_temp;

/*! \brief Create templates for paint types.
 *
 * This function is only called one time for every execution.
 */
static void
xnjsmb_init_paints(void) {
    Handle<FunctionTemplate> temp, meth;
    Handle<ObjectTemplate> inst_temp;
    Handle<ObjectTemplate> proto_temp;
    
    /*
     * Base type of paint types.
     */
    temp = FunctionTemplate::New();
    xnjsmb_paint_temp = Persistent<FunctionTemplate>::New(temp);
    xnjsmb_paint_temp->SetClassName(String::New("paint"));
    
    meth = FunctionTemplate::New(xnjsmb_paint_fill);
    proto_temp = xnjsmb_paint_temp->PrototypeTemplate();
    SET(proto_temp, "fill", meth);

    meth = FunctionTemplate::New(xnjsmb_paint_stroke);
    proto_temp = xnjsmb_paint_temp->PrototypeTemplate();
    SET(proto_temp, "stroke", meth);

    /*
     * Paint color
     */
    temp = FunctionTemplate::New(xnjsmb_paint_color);
    xnjsmb_paint_color_temp = Persistent<FunctionTemplate>::New(temp);
    xnjsmb_paint_color_temp->SetClassName(String::New("paint_color"));
    xnjsmb_paint_color_temp->Inherit(xnjsmb_paint_temp);

    inst_temp = xnjsmb_paint_color_temp->InstanceTemplate();
    inst_temp->SetInternalFieldCount(1);

    temp = FunctionTemplate::New(xnjsmb_paint_color_new);
    xnjsmb_paint_color_new_temp = Persistent<FunctionTemplate>::New(temp);
    
    /*
     * Paint image
     */
    temp = FunctionTemplate::New(xnjsmb_paint_image);
    xnjsmb_paint_image_temp = Persistent<FunctionTemplate>::New(temp);
    xnjsmb_paint_image_temp->SetClassName(String::New("paint_image"));
    xnjsmb_paint_image_temp->Inherit(xnjsmb_paint_temp);
    
    inst_temp = xnjsmb_paint_image_temp->InstanceTemplate();
    inst_temp->SetInternalFieldCount(1);

    temp = FunctionTemplate::New(xnjsmb_paint_image_new);
    xnjsmb_paint_image_new_temp = Persistent<FunctionTemplate>::New(temp);
}

void xnjsmb_paints_init_mb_rt_temp(Handle<FunctionTemplate> rt_temp) {
    static int init_flag = 0;
    Handle<ObjectTemplate> rt_proto_temp;

    if(!init_flag) {
	xnjsmb_init_paints();
	init_flag = 1;
    }

    rt_proto_temp = rt_temp->PrototypeTemplate();
    SET(rt_proto_temp, "paint_color_new", xnjsmb_paint_color_new_temp);
    SET(rt_proto_temp, "paint_image_new", xnjsmb_paint_image_new_temp);
}
