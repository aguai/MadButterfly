#include <v8.h>

extern "C" {
#include "mb.h"
}

#include "mbfly_njs.h"

using namespace v8;

#ifndef ASSERT
#define ASSERT(x)
#endif


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

/*! \brief Create templates for paint types.
 *
 * This function is only called one time for every execution.
 */
static void
xnjsmb_init_paints(void) {
    Handle<FunctionTemplate> temp;
    Handle<ObjectTemplate> inst_temp;
    
    /*
     * Base type of paint types.
     */
    temp = FunctionTemplate::New();
    xnjsmb_paint_temp = Persistent<FunctionTemplate>::New(temp);
    xnjsmb_paint_temp->SetClassName(String::New("paint"));

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
}
