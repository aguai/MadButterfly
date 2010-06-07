#include <stdio.h>
#include <v8.h>

extern "C" {
#include "X_supp_njs.h"
}

#include "mbfly_njs.h"

using namespace v8;

/*! \defgroup njs_template_cb Callback functions for v8 engine and nodejs.
 *
 * @{
 */

/*! \brief to Create a njs runtime object for MadButterfly.
 *
 * Three arguments are requried.  They are
 *   - display name,
 *   - width, and
 *   - height.
 */
static Handle<Value>
xnjsmb_new(const Arguments &args) {
    HandleScope scope;
    int argc;
    Handle<Value> exc;
    njs_runtime_t *rt;
    char *display_name;
    int width, height;
    Handle<Object> self;

    argc = args.Length();
    if(argc != 3) {
	exc = Exception::Error(String::New("Need 3 arguments."));
	return ThrowException(exc);
    }

    if(!args[0]->IsString() || !args[1]->IsInt32() || !args[2]->IsInt32()) {
	exc = Exception::Error(String::New("Invalid argument type."));
	return ThrowException(exc);
    }
    
    String::Utf8Value disp_utf8(args[0]->ToString());
    display_name = *disp_utf8;
    width = args[1]->Int32Value();
    height = args[2]->Int32Value();
    rt = X_njs_MB_new(display_name, width, height);

    self = args.This();
    WRAP(self, rt);
    
    X_njs_MB_init_handle_connection(rt);

    return Null();
}

static Handle<Value>
xnjsmb_handle_connection(const Arguments &args) {
}

/* @} */

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
    Handle<FunctionTemplate> func;
    Handle<ObjectTemplate> rt_obj_temp;

    func = FunctionTemplate::New(hello_func);
    target->Set(String::New("Hello"), func->GetFunction());

    func = FunctionTemplate::New(xnjsmb_new);
    target->Set(String::New("mb_rt"), func->GetFunction());
    rt_obj_temp = func->PrototypeTemplate();
    rt_obj_temp->SetInternalFieldCount(1);
    
    func = FunctionTemplate::New(xnjsmb_coord_new);
    SET(rt_obj_temp, "coord_new", func);
}
