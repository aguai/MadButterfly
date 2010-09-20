// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <v8.h>
#include "mbfly_njs.h"

#include <string.h>

extern "C" {
#include <mb.h>
}

#ifndef ASSERT
#define ASSERT(x)
#endif

#define OK 0

using namespace v8;

/*! \defgroup xnjsmb_shapes JS binding for shapes.
 * \ingroup xnjsmb
 *
 * @{
 */
/*! \brief This function is called when GC collecting a shape.
 *
 * It was installed by Persistent<Object>::MakeWeak().
 */
static void
xnjsmb_shape_recycled(Persistent<Value> obj, void *parameter) {
    Persistent<Object> *self_hdl = (Persistent<Object> *)parameter;
    Handle<Object> js_rt;
    redraw_man_t *rdman;
    shape_t *shape;

    shape = (shape_t *)UNWRAP(*self_hdl);
    if(shape == NULL)
	return;

    WRAP(*self_hdl, NULL);

    js_rt = GET(*self_hdl, "mbrt")->ToObject();
    rdman = xnjsmb_rt_rdman(js_rt);
    rdman_shape_changed(rdman, shape);
    rdman_shape_free(rdman, shape);

    self_hdl->Dispose();
    delete self_hdl;
}

static void
xnjsmb_shape_mod(Handle<Object> self, shape_t *sh) {
    Persistent<Object> *self_hdl;
    static int count = 0;

    /* Keep associated js object in property store for retrieving,
     * later, without create new js object.
     */
    self_hdl = new Persistent<Object>();
    *self_hdl = Persistent<Object>::New(self);
    mb_prop_set(&sh->obj.props, PROP_JSOBJ, self_hdl);

    self_hdl->MakeWeak(self_hdl, xnjsmb_shape_recycled);

    /* XXX: should be remove.  It is for trace recycle of shape */
    count++;
    if(count > 10000) {
	V8::LowMemoryNotification();
	count = 0;
    }
}

/*! \brief Set style blocks for a stext object from JS.
 *
 * A style block is style setting of a chip of text.  It is a 3-tuple,
 * includes number of charaters, a font face, and font size.  This
 * function need a list of 3-tuples to set style of text chips of the
 * stext.
 */
static void
xnjsmb_sh_stext_set_style(shape_t *sh, Handle<Object> self,
			  Handle<Value> blks, const char **err) {
    Array *blksobj;
    Array *blkobj;
    mb_style_blk_t *mb_blks;
    int nblks;
    Handle<Object> rt;
    redraw_man_t *rdman;
    int r;
    int i;

    blksobj = Array::Cast(*blks);
    nblks = blksobj->Length();
    mb_blks = new mb_style_blk_t[nblks];
    for(i = 0; i < nblks; i++) {
	blkobj = Array::Cast(*blksobj->Get(i));
	mb_blks[i].n_chars = blkobj->Get(0)->ToInt32()->Value();
	mb_blks[i].face = (mb_font_face_t *)UNWRAP(blkobj->Get(1)->ToObject());
	mb_blks[i].font_sz = blkobj->Get(2)->ToNumber()->Value();
    }

    r = sh_stext_set_style(sh, mb_blks, nblks);
    if(r != 0) {
	*err = "Unknown error";
	return;
    }

    /*
     * Mark changed.
     */
    rt = GET(self, "mbrt")->ToObject();
    ASSERT(rt != NULL);
    rdman = xnjsmb_rt_rdman(rt);

    if(sh_get_coord(sh))
	rdman_shape_changed(rdman, sh);

    delete mb_blks;
}

static Handle<Value>
xnjsmb_shape_stroke_width_get(Handle<Object> self, shape_t *sh,
			      const char **err) {
    float stroke_width;

    stroke_width = sh_get_stroke_width(sh);
    return Number::New(stroke_width);
}

static void
xnjsmb_shape_stroke_width_set(Handle<Object> self, shape_t *sh,
			      Handle<Value> value, const char **err) {
    float stroke_width;
    Handle<Object> rt;
    redraw_man_t *rdman;

    stroke_width = value->Int32Value();
    sh_set_stroke_width(sh, stroke_width);

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
xnjsmb_shape_show(shape_t *sh, Handle<Object> self) {
    Handle<Object> js_rt;
    redraw_man_t *rdman;

    js_rt = GET(self, "mbrt")->ToObject();
    ASSERT(js_rt != NULL);
    rdman = xnjsmb_rt_rdman(js_rt);

    sh_show(sh);
    rdman_shape_changed(rdman, sh);
}

static void
xnjsmb_shape_hide(shape_t *sh, Handle<Object> self) {
    Handle<Object> js_rt;
    redraw_man_t *rdman;

    js_rt = GET(self, "mbrt")->ToObject();
    ASSERT(js_rt != NULL);
    rdman = xnjsmb_rt_rdman(js_rt);

    sh_hide(sh);
    rdman_shape_changed(rdman, sh);
}

static void
xnjsmb_shape_remove(shape_t *sh, Handle<Object> self) {
    Handle<Object> js_rt;
    redraw_man_t *rdman;
    Persistent<Object> *self_hdl;
    int r;

    self_hdl = (Persistent<Object> *)mb_prop_get(&sh->obj.props,
						 PROP_JSOBJ);

    SET(*self_hdl, "valid", Boolean::New(0));
    WRAP(*self_hdl, NULL);

    js_rt = GET(*self_hdl, "mbrt")->ToObject();
    ASSERT(js_rt != NULL);
    rdman = xnjsmb_rt_rdman(js_rt);

    rdman_shape_changed(rdman, sh);
    r = rdman_shape_free(rdman, sh);
    if(r != OK)
	THROW_noret("Can not free a shape for unknown reason");

    self_hdl->Dispose();
    delete self_hdl;
}

static void
xnjsmb_sh_rect_set(shape_t *sh, Handle<Object> self, float x, float y,
		   float w, float h, float rx, float ry) {
    Handle<Object> rt;
    redraw_man_t *rdman;

    sh_rect_set(sh, x, y, w, h, rx, ry);

    /*
     * Mark changed.
     */
    rt = GET(self, "mbrt")->ToObject();
    ASSERT(rt != NULL);
    rdman = xnjsmb_rt_rdman(rt);

    if(sh_get_coord(sh))
	rdman_shape_changed(rdman, sh);
}

/* @} */

#include "shapes-inc.h"

/*! \defgroup xnjsmb_shapes_wraps Exported wrapper makers for shapes
 * \ingroup xnjsmb_shapes
 *
 * These functions are used by methods of mb_rt to wrap shape objects
 * as Javascript objects.
 *
 * @{
 */
Handle<Value>
export_xnjsmb_auto_path_new(shape_t *sh) {
    return xnjsmb_auto_path_new(sh);
}

Handle<Value>
export_xnjsmb_auto_stext_new(shape_t *sh) {
    return xnjsmb_auto_stext_new(sh);
}

Handle<Value>
export_xnjsmb_auto_image_new(shape_t *sh) {
    return xnjsmb_auto_image_new(sh);
}

Handle<Value>
export_xnjsmb_auto_rect_new(shape_t *sh) {
    return xnjsmb_auto_rect_new(sh);
}

/* @} */

/*! \defgroup xnjsmb_shapes_cons Constructor of shapes
 * \ingroup xnjsmb_shapes
 *
 * @{
 */
shape_t *
xnjsmb_path_new(njs_runtime_t *rt, const char *d) {
    redraw_man_t *rdman;
    shape_t *sh;

    rdman = X_njs_MB_rdman(rt);
    sh = rdman_shape_path_new(rdman, d);
    /* Code generator supposes that callee should free the memory */
    free((void *)d);

    return sh;
}

shape_t *
xnjsmb_stext_new(njs_runtime_t *rt, const char *txt, float x, float y) {
    redraw_man_t *rdman;
    shape_t *sh;

    rdman = X_njs_MB_rdman(rt);
    sh = rdman_shape_stext_new(rdman, txt, x, y);
    /* Code generator supposes that callee should free the memory */
    free((void *)txt);

    return sh;
}

shape_t *
xnjsmb_image_new(njs_runtime_t *rt, float x, float y, float w, float h) {
    redraw_man_t *rdman;
    shape_t *sh;

    rdman = X_njs_MB_rdman(rt);
    sh = rdman_shape_image_new(rdman, x, y, w, h);

    return sh;
}

shape_t *
xnjsmb_rect_new(njs_runtime_t *rt, float x, float y, float w, float h,
		float rx, float ry, const char **err) {
    redraw_man_t *rdman;
    shape_t *sh;

    rdman = X_njs_MB_rdman(rt);
    sh = rdman_shape_rect_new(rdman, x, y, w, h, rx, ry);
    if(sh == NULL) {
	*err = "Can not create a sh_rect_t";
	return NULL;
    }

    return sh;
}

/* @} */

/*! \brief Set properties of template of mb_rt.
 * \ingroup xnjsmb_shapes
 */
void
xnjsmb_shapes_init_mb_rt_temp(Handle<FunctionTemplate> rt_temp) {
    HandleScope scope;
    Handle<FunctionTemplate> path_new_temp, stext_new_temp;
    Handle<FunctionTemplate> image_new_temp;
    Handle<ObjectTemplate> rt_proto_temp;
    static int temp_init_flag = 0;

    if(temp_init_flag == 0) {
	xnjsmb_auto_shape_init();
	xnjsmb_auto_path_init();
	xnjsmb_auto_stext_init();
	xnjsmb_auto_image_init();
	xnjsmb_auto_rect_init();
	temp_init_flag = 1;
    }
    return;
}
