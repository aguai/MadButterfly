#include <SkCanvas.h>

#define DECL_C extern "C" {
#define END_C }

DECL_C

#include <stdio.h>
#include <mb.h>
#include <jni.h>


jint Java_org_madbutterfly__1jni_redraw_1man_1new(JNIEnv *env,
						  jobject cr,
						  jobject backend) {
    jclass cls;
    jfieldID fid;
    SkCanvas *_cr, *_backend;
    redraw_man_t *rdman;
    
    cls = env->GetObjectClass(cr);
    fid = env->GetFieldID(cls, "mNativeCanvas", "I");
    _cr = (SkCanvas *)env->GetIntField(cr, fid);
    _backend = (SkCanvas *)env->GetIntField(backend, fid);
    rdman = (redraw_man_t *)malloc(sizeof(redraw_man_t));
    redraw_man_init(rdman, (mbe_t *)_cr, (mbe_t *)_backend);

    return (jint)rdman;
}

void Java_org_madbutterfly__1jni_redraw_1man_1free(JNIEnv *env,
						   jint rdman) {
    redraw_man_destroy((redraw_man_t *)rdman);
    free((redraw_man_t *)rdman);
}

jint Java_org_madbutterfly__1jni_rdman_1add_1shape(JNIEnv *env,
						   jint rdman,
						   jint shape,
						   jint coord) {
    jint r;
    
    r = rdman_add_shape((redraw_man_t *)rdman,
			(shape_t *)shape,
			(coord_t *)coord);
    return r;
}

jint Java_org_madbutterfly__1jni_rdman_1get_1root(JNIEnv *env,
						  jint rdman) {
    jint root;

    root = (jint)rdman_get_root((redraw_man_t *)rdman);
    return root;
}

jint Java_org_madbutterfly__1jni_rdman_1redraw_1all(JNIEnv *env,
						    jint rdman) {
    jint r;

    r = rdman_redraw_all((redraw_man_t *)rdman);
    return r;
}

void Java_org_madbutterfly__1jni_rdman_1paint_1fill(JNIEnv *env,
						    jint rdman,
						    jint paint,
						    jint shape) {
    rdman_paint_fill((redraw_man_t *)rdman,
		     (paint_t *)paint,
		     (shape_t *)shape);
}

void Java_org_madbutterfly__1jni_rdman_1paint_1stroke(JNIEnv *env,
						      jint rdman,
						      jint paint,
						      jint shape) {
    rdman_paint_stroke((redraw_man_t *)rdman,
		       (paint_t *)paint,
		       (shape_t *)shape);
}

jint Java_org_madbutterfly__1jni_rdman_1coord_1new(JNIEnv *env,
						   jint rdman,
						   jint parent) {
    jint coord;

    coord = (jint)rdman_coord_new((redraw_man_t *)rdman,
				  (coord_t *)parent);
    return coord;
}

void Java_org_madbutterfly__1jni_rdman_1coord_1free(JNIEnv *env,
						    jint rdman,
						    jint coord) {
    rdman_coord_free((redraw_man_t *)rdman,
		     (coord_t *)coord);
}

void Java_org_madbutterfly__1jni_rdman_1coord_1subtree_1free(JNIEnv *env,
							     jint rdman,
							     jint coord) {
    rdman_coord_subtree_free((redraw_man_t *)rdman,
			     (coord_t *)coord);
}

void Java_org_madbutterfly__1jni_rdman_1coord_1changed(JNIEnv *env,
						       jint rdman,
						       jint coord) {
    rdman_coord_changed((redraw_man_t *)rdman,
			(coord_t *)coord);
}

void Java_org_madbutterfly__1jni_rdman_1shape_1changed(JNIEnv *env,
						       jint rdman,
						       jint shape) {
    rdman_shape_changed((redraw_man_t *)rdman,
			(shape_t *)shape);
}

void Java_org_madbutterfly__1jni_rdman_1shape_1free(JNIEnv *env,
						    jint rdman,
						    jint shape) {
    rdman_shape_free((redraw_man_t *)rdman,
		     (shape_t *)shape);
}

jint Java_org_madbutterfly__1jni_rdman_1shape_1path_1new(JNIEnv *env,
							 jint rdman,
							 jstring data) {
    const char *str;
    jint shape;

    str = env->GetStringUTFChars(data, NULL);
    shape = (jint)rdman_shape_path_new((redraw_man_t *)rdman, (char *)str);
    env->ReleaseStringUTFChars(data, str);

    return shape;
}

jint Java_org_madbutterfly__1jni_rdman_1paint_1color_1new(JNIEnv *env,
							  jint rdman,
							  jfloat r, jfloat g,
							  jfloat b, jfloat a) {
    jint paint;

    paint = (jint)rdman_paint_color_new((redraw_man_t *)rdman,
					r, g, b, a);
    return paint;
}

jint Java_org_madbutterfly__1jni_rdman_1paint_1free(JNIEnv *env,
						    jint rdman,
						    jint paint) {
    jint r;

    r = rdman_paint_free((redraw_man_t *)rdman,
			 (paint_t *)paint);
    return r;
}

void Java_org_madbutterfly__1jni_paint_1color_1set(JNIEnv *env,
						   jint paint,
						   jfloat r, jfloat g,
						   jfloat b, jfloat a) {
    paint_color_set((paint_t *)paint, r, g, b, a);
}

jfloatArray Java_org_madbutterfly__1jni_paint_1color_1get(JNIEnv *env,
							  jint paint) {
    co_comp_t r, g, b, a;
    jfloat color[4];
    jfloatArray result;

    paint_color_get((paint_t *)paint, &r, &g, &b, &a);
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;
    
    result = env->NewFloatArray(4);
    env->SetFloatArrayRegion(result, 0, 4, color);

    return result;
}

END_C

