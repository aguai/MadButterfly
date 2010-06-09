#include <v8.h>

extern "C" {
#include "mb.h"
}

#include "mbfly_njs.h"

using namespace v8;


/*! \brief Base type of all types of paints.
 */
static Handle<Value>
xnjsmb_paint(const Arguments &args) {
}

/*! \brief Constructor of color paint_color_t object for Javascript.
 */
static Handle<Value>
xnjsmb_paint_color(const Arguments &args) {
}

static Persistent<FunctionTemplate> paint_temp;
static Persistent<FunctionTemplate> paint_color_temp;

/*! \brief Create and return a paint_color object.
 */
static Handle<Value>
xnjsmb_paint_color_new(const Arguments &args) {
    HandleScope scope;
    Handle<Object> rt = args.This();
    Handle<Object> paint_color_obj;
    Handle<Function> paint_color_func;
    Handle<Value> pc_args[4];
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
    paint_color_func = paint_color_temp->GetFunction();
    paint_color_obj = paint_color_func->NewInstance(1, pc_args);

    scope.Close(paint_color_obj);
    return paint_color_obj;
}

static Persistent<FunctionTemplate> paint_color_new_temp;

/*! \brief Create templates for paint types.
 *
 * This function is only called one time for every execution.
 */
static void
xnjsmb_init_paints(void) {
    Handle<FunctionTemplate> temp;
    
    temp = FunctionTemplate::New(xnjsmb_paint);
    paint_temp = Persistent<FunctionTemplate>::New(temp);
    paint_temp->SetClassName(String::New("paint"));

    temp = FunctionTemplate::New(xnjsmb_paint_color);
    paint_color_temp = Persistent<FunctionTemplate>::New(temp);
    paint_color_temp->SetClassName(String::New("paint_color"));
    paint_color_temp->Inherit(paint_temp);

    temp = FunctionTemplate::New(xnjsmb_paint_color_new);
    paint_color_new_temp = Persistent<FunctionTemplate>::New(temp);
}

void xnjsmb_paints_init_mb_rt_temp(Handle<FunctionTemplate> rt_temp) {
    static int init_flag = 0;
    Handle<ObjectTemplate> rt_proto_temp;

    if(!init_flag) {
	xnjsmb_init_paints();
	init_flag = 1;
    }

    rt_proto_temp = rt_temp->PrototypeTemplate();
    SET(rt_proto_temp, "paint_color_new", paint_color_new_temp);
}
