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
    
    if(sh_get_coord(sh))
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

/*! \defgroup stext_path Template for stext objects.
 *
 * @{
 */

/*! \brief Constructor for stext objects.
 *
 * 4 arguments
 * \param rt is a runtime object.
 * \param data is a text to be showed.
 * \param x is postion in x-axis.
 * \param y is position in y-axis.
 */
static Handle<Value>
xnjsmb_shape_stext(const Arguments &args) {
    int argc = args.Length();
    Handle<Object> self = args.This();
    Handle<Object> rt;
    float x, y;
    char *data;
    redraw_man_t *rdman;
    shape_t *stext;
    
    if(argc != 4)
	THROW("Invalid number of arguments (!= 4)");
    if(!args[0]->IsObject() || !args[1]->IsString() ||
       !args[2]->IsNumber() || !args[3]->IsNumber())
	THROW("Invalid argument type");

    rt = args[0]->ToObject();
    String::Utf8Value data_utf8(args[1]);
    data = *data_utf8;
    x = args[2]->ToNumber()->Value();
    y = args[3]->ToNumber()->Value();

    rdman = xnjsmb_rt_rdman(rt);
    ASSERT(rdman != NULL);

    stext = rdman_shape_stext_new(rdman, data, x, y);

    WRAP(self, stext);
    SET(self, "mbrt", rt);

    return Null();
}

static Persistent<FunctionTemplate> xnjsmb_shape_stext_temp;

/*! \brief Create a stext and return it.
 */
static Handle<Value>
xnjsmb_shape_stext_new(const Arguments &args) {
    HandleScope scope;
    int argc = args.Length();
    Handle<Object> self = args.This();
    Handle<Value> stext_args[4];
    Handle<Object> stext_obj;
    Handle<Function> func;

    if(argc != 3)
	THROW("Invalid number of arguments (!= 3)");

    stext_args[0] = self;
    stext_args[1] = args[0];
    stext_args[2] = args[1];
    stext_args[3] = args[2];

    func = xnjsmb_shape_stext_temp->GetFunction();
    stext_obj = func->NewInstance(4, stext_args);
    ASSERT(stext_obj != NULL);

    scope.Close(stext_obj);
    return stext_obj;
}

/*! \brief Setup style blocks for a stext.
 *
 * It defines font style and size for blocks of text message.
 *
 * \param blks is a list (n_char, face, font size) tuples.
 */
static Handle<Value>
xnjsmb_shape_stext_set_style(const Arguments &args) {
    HandleScope scope;
    int argc = args.Length();
    Handle<Object> self = args.This();
    shape_t *sh;
    Array *blksobj;
    Array *blkobj;
    mb_style_blk_t *blks;
    int nblks;
    int i;
    int r;

    if(argc != 1)
	THROW("Invalid number of arguments (!= 1)");
    if(!args[0]->IsArray())
	THROW("Invalid type of the argument");

    blksobj = Array::Cast(*args[0]);
    nblks = blksobj->Length();
    blks = new mb_style_blk_t[nblks];
    for(i = 0; i < nblks; i++) {
	blkobj = Array::Cast(*blksobj->Get(i));
	blks[i].n_chars = blkobj->Get(0)->ToInt32()->Value();
	blks[i].face = (mb_font_face_t *)UNWRAP(blkobj->Get(1)->ToObject());
	blks[i].font_sz = blkobj->Get(2)->ToNumber()->Value();
    }
    
    sh = (shape_t *)UNWRAP(self);
    r = sh_stext_set_style(sh, blks, nblks);
    if(r != 0)
	THROW("Unknown error");
    
    delete blks;
    
    return Null();
}

/*! \brief Initialize function template for stext objects.
 */
static void
xnjsmb_init_stext_temp(void) {
    Handle<FunctionTemplate> func_temp;
    Handle<FunctionTemplate> meth_temp;
    Handle<ObjectTemplate> inst_temp;
    Handle<ObjectTemplate> proto_temp;
    
    func_temp = FunctionTemplate::New(xnjsmb_shape_stext);
    func_temp->Inherit(xnjsmb_shape_temp);
    func_temp->SetClassName(String::New("stext"));
    
    inst_temp = func_temp->InstanceTemplate();
    inst_temp->SetInternalFieldCount(1);

    proto_temp = func_temp->PrototypeTemplate();
    meth_temp = FunctionTemplate::New(xnjsmb_shape_stext_set_style);
    SET(proto_temp, "set_style", meth_temp);
    
    xnjsmb_shape_stext_temp = Persistent<FunctionTemplate>::New(func_temp);
}

/* @} */

/*! \defgroup image_temp Templates for image objects.
 *
 * @{
 */

static Handle<Value>
xnjsmb_shape_image(const Arguments &args) {
    int argc = args.Length();
    Handle<Object> self = args.This();
    Handle<Object> rt;
    float x, y;
    float w, h;
    redraw_man_t *rdman;
    shape_t *img;

    if(argc != 5)
	THROW("Invalid number of arguments (!= 5)");
    if(!args[0]->IsObject() ||
       !args[1]->IsNumber() || !args[2]->IsNumber() ||
       !args[3]->IsNumber() || !args[4]->IsNumber())
	THROW("Invalid argument type");

    rt = args[0]->ToObject();
    x = args[1]->ToNumber()->Value();
    y = args[2]->ToNumber()->Value();
    w = args[3]->ToNumber()->Value();
    h = args[4]->ToNumber()->Value();

    rdman = xnjsmb_rt_rdman(rt);
    img = rdman_shape_image_new(rdman, x, y, w, h);

    WRAP(self, img);
    SET(self, "mbrt", rt);

    return Null();
}

static Persistent<FunctionTemplate> xnjsmb_shape_image_temp;

static Handle<Value>
xnjsmb_shape_image_new(const Arguments &args) {
    HandleScope scope;
    int argc = args.Length();
    Handle<Object> self = args.This();
    Handle<Value> img_args[5];
    Handle<Object> img_obj;
    Handle<Function> func;

    if(argc != 4)
	THROW("Invalid number of arguments (!= 3)");

    img_args[0] = self;
    img_args[1] = args[0];
    img_args[2] = args[1];
    img_args[3] = args[2];
    img_args[4] = args[3];

    func = xnjsmb_shape_image_temp->GetFunction();
    img_obj = func->NewInstance(5, img_args);
    ASSERT(img_obj != NULL);

    scope.Close(img_obj);
    return img_obj;
}

static void
xnjsmb_init_image_temp() {
    Handle<FunctionTemplate> func_temp;
    Handle<FunctionTemplate> meth_temp;
    Handle<ObjectTemplate> inst_temp;
    Handle<ObjectTemplate> proto_temp;
    
    func_temp = FunctionTemplate::New(xnjsmb_shape_image);
    func_temp->Inherit(xnjsmb_shape_temp);
    func_temp->SetClassName(String::New("image"));
    
    inst_temp = func_temp->InstanceTemplate();
    inst_temp->SetInternalFieldCount(1);

    xnjsmb_shape_image_temp = Persistent<FunctionTemplate>::New(func_temp);
}

/* @} */

/*! \brief Set properties of template of mb_rt.
 */
void
xnjsmb_shapes_init_mb_rt_temp(Handle<FunctionTemplate> rt_temp) {
    HandleScope scope;
    Handle<FunctionTemplate> path_new_temp, stext_new_temp;
    Handle<FunctionTemplate> image_new_temp;
    Handle<ObjectTemplate> rt_proto_temp;
    static int temp_init_flag = 0;

    if(temp_init_flag == 0) {
	xnjsmb_init_shape_temp();
	xnjsmb_init_path_temp();
	xnjsmb_init_stext_temp();
	temp_init_flag = 1;
    }

    rt_proto_temp = rt_temp->PrototypeTemplate();
    
    path_new_temp = FunctionTemplate::New(xnjsmb_shape_path_new);
    SET(rt_proto_temp, "path_new", path_new_temp);

    stext_new_temp = FunctionTemplate::New(xnjsmb_shape_stext_new);
    SET(rt_proto_temp, "stext_new", stext_new_temp);

    image_new_temp = FunctionTemplate::New(xnjsmb_shape_image_new);
    SET(rt_proto_temp, "image_new", image_new_temp);
}
