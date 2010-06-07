#include <v8.h>
#include "mbfly_njs.h"

extern "C" {
#include <mb.h>
}

using namespace v8;

/*! \defgroup shape_temp Templates for shape and derive.
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
}

/* @} */

