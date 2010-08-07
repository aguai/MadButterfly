#include <v8.h>
#include "mbfly_njs.h"

#include <string.h>

extern "C" {
#include <mb.h>
}

#ifndef ASSERT
#define ASSERT(x)
#endif

using namespace v8;

/*! \defgroup xnjsmb_shapes Implement binding of shapes.
 * \ingroup xnjsmb
 *
 * @{
 */
static void
xnjsmb_sh_stext_set_style(shape_t *sh, Handle<Value> blks, const char **err) {
    Array *blksobj;
    Array *blkobj;
    mb_style_blk_t *mb_blks;
    int nblks;
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

/* @} */

#include "shapes-inc.h"

/*! \defgroup xnjsmb_shapes_export Export functions of creating objects.
 * \ingroup xnjsmb_shapes
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

/* @} */

/*! \defgroup xnjsmb_shapes_wraps Wrap functions for shape objects.
 * \ingroup xnjsmb_shapes
 * 
 * These functions are used by methods of mb_rt to wrap shape objects
 * as Javascript objects.
 *
 * @{
 */
shape_t *
xnjsmb_path_new(njs_runtime_t *rt, const char *d) {
    redraw_man_t *rdman;
    shape_t *sh;
    
    rdman = X_njs_MB_rdman(rt);
    sh = rdman_shape_path_new(rdman, d);
    
    return sh;
}

shape_t *
xnjsmb_stext_new(njs_runtime_t *rt, const char *txt, float x, float y) {
    redraw_man_t *rdman;
    shape_t *sh;

    rdman = X_njs_MB_rdman(rt);
    sh = rdman_shape_stext_new(rdman, txt, x, y);

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
	temp_init_flag = 1;
    }
    return;
}
