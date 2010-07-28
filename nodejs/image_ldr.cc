/*! \file
 * This file implements Javascript binding for img_ldr_t of MadButterfly.
 */
#include <v8.h>

extern "C" {
#include "mb.h"
}

#include "mbfly_njs.h"

using namespace v8;

#ifndef ASSERT
#define ASSERT(x)
#endif

/*! \defgroup img_ldr_js Javascript binding for image loader.
 *
 * @{
 */

static Persistent<ObjectTemplate> img_data_temp;

/*! \brief load() method of img_ldr Javascript objects.
 */
static Handle<Value>
xnjsmb_img_ldr_load(const Arguments &args) {
    HandleScope scope;
    int argc = args.Length();
    Handle<Object> self = args.This();
    char *img_id;
    mb_img_ldr_t *img_ldr;
    mb_img_data_t *img_data;
    Handle<Object> img_data_obj;

    if(argc != 1)
	THROW("Invalid number of arguments (!= 1)");
    if(!args[0]->IsString())
	THROW("Invalid argument type");

    String::Utf8Value img_id_utf8(args[0]->ToString());
    img_id = *img_id_utf8;
    img_ldr = (mb_img_ldr_t *)UNWRAP(self);

    img_data_obj = img_data_temp->NewInstance();
    ASSERT(img_data_obj);
    
    img_data = MB_IMG_LDR_LOAD(img_ldr, img_id);
    if(img_data == NULL)
	THROW("Can not load an image");
    WRAP(img_data_obj, img_data);

    scope.Close(img_data_obj);
    
    return img_data_obj;
}

/*! \brief Constructor function of img_ldr Javascript objects.
 */
static Handle<Value>
xnjsmb_img_ldr(const Arguments &args) {
    HandleScope scope;
    int argc = args.Length();
    Handle<Object> self = args.This();
    char *path;
    mb_img_ldr_t *img_ldr;
    
    if(argc != 1)
	THROW("Invalid number of arguments (!= 1)");
    if(!args[0]->IsString())
	THROW("Invalid argument type");
    
    String::Utf8Value pathutf8(args[0]->ToString());
    path = *pathutf8;

    img_ldr = simple_mb_img_ldr_new(path);
    if(img_ldr == NULL)
	THROW("Can not create an image loader");
    WRAP(self, img_ldr);

    return Null();
}

static Persistent<FunctionTemplate> xnjsmb_img_ldr_temp;

static Handle<Value>
xnjsmb_img_ldr_new(const Arguments &args) {
    HandleScope scope;
    int argc = args.Length();
    Handle<Value> il_args[1];
    Handle<Object> img_ldr;
    Handle<Function> func;
    
    if(argc != 1)
	THROW("Invalid number of arguments (!= 1)");
    if(!args[0]->IsString())
	THROW("Invalid argument type");

    il_args[0] = args[0];
    func = xnjsmb_img_ldr_temp->GetFunction();
    img_ldr = func->NewInstance(1, il_args);

    scope.Close(img_ldr);
    return img_ldr;
}

/* @} */

/*! \brief Initialize image loader.
 *
 * This function is called by init() in mbfly_njs.cc when the module
 * being loaded.
 */
void
xnjsmb_img_ldr_init_mb_rt_temp(Handle<Object> rt_temp) {
    HandleScope scope;
    Handle<FunctionTemplate> img_ldr_temp;
    Handle<FunctionTemplate> img_ldr_new_temp;
    Handle<ObjectTemplate> ldr_inst_temp;
    Handle<ObjectTemplate> ldr_proto_temp;
    Handle<FunctionTemplate> img_ldr_load_temp;
    Handle<ObjectTemplate> _img_data_temp;

    /* Setup object template for img_data_t object for Javascript */
    _img_data_temp = ObjectTemplate::New();
    _img_data_temp->SetInternalFieldCount(1);
    img_data_temp = Persistent<ObjectTemplate>::New(_img_data_temp);
    
    /* Setup img_ldr class */
    img_ldr_temp = FunctionTemplate::New(xnjsmb_img_ldr);
    img_ldr_temp->SetClassName(String::New("img_ldr"));
    ldr_inst_temp = img_ldr_temp->InstanceTemplate();
    ldr_inst_temp->SetInternalFieldCount(1);

    /* Set method load() for img_ldr */
    ldr_proto_temp = img_ldr_temp->PrototypeTemplate();
    img_ldr_load_temp = FunctionTemplate::New(xnjsmb_img_ldr_load);
    SET(ldr_proto_temp, "load", img_ldr_load_temp);

    xnjsmb_img_ldr_temp = Persistent<FunctionTemplate>::New(img_ldr_temp);

    /* Initialize img_ldr_new function */
    img_ldr_new_temp = FunctionTemplate::New(xnjsmb_img_ldr_new);
    SET(rt_temp, "img_ldr_new", img_ldr_new_temp->GetFunction());
}
