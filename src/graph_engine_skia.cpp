#include <stdio.h>
#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkShader.h>

#define C_START extern "C" {
#define C_END }

C_START

#include "mb_graph_engine_skia.h"
#include "mb_shapes.h"

struct _mbe_scaled_font_t {
    struct _mb_font_face_t *face;
    co_aix fnt_mtx[6];
    co_aix ctm[6];
};
struct _mbe_font_face_t {};
struct _mbe_t {
    SkCanvas *canvas;
    SkShader *shader;
    int shader_owned;
};

#ifndef ASSERT
#define ASSERT(x)
#endif

void mbe_pattern_add_color_stop_rgba(mbe_pattern_t *ptn,
					    co_aix offset,
					    co_aix r, co_aix g, co_aix b,
					    co_aix a) {}
mbe_pattern_t *mbe_pattern_create_for_surface(mbe_surface_t *surface) {}
mbe_pattern_t *mbe_pattern_create_radial(co_aix cx0, co_aix cy0,
						co_aix radius0,
						co_aix cx1, co_aix cy1,
						co_aix radius1) {}
mbe_pattern_t *mbe_pattern_create_linear(co_aix x0, co_aix y0,
						co_aix x1, co_aix y1) {}
void mbe_pattern_set_matrix(mbe_pattern_t *ptn,
				   const co_aix matrix[6]) {}
void mbe_pattern_destroy(mbe_pattern_t *canvas) {}

int mbe_image_surface_get_stride(mbe_surface_t *surface) {}
int mbe_image_surface_get_height(mbe_surface_t *surface) {}
int mbe_image_surface_get_width(mbe_surface_t *surface) {}
unsigned char *mbe_image_surface_get_data(mbe_surface_t *surface) {}
mbe_surface_t *mbe_image_surface_create_from_png(const char *filename) {}
mbe_surface_t *
mbe_image_surface_create_for_data(unsigned char *data,
				  mb_img_fmt_t fmt,
				  int width, int height,
				  int stride) {}
mb_img_fmt_t mbe_image_surface_get_format(mbe_surface_t *surface) {}
mbe_surface_t *
mbe_image_surface_create(mb_img_fmt_t fmt, int width, int height) {}

mbe_scaled_font_t *mbe_scaled_font_reference(mbe_scaled_font_t *scaled) {}
void mbe_scaled_font_destroy(mbe_scaled_font_t *scaled) {}
mbe_font_face_t *mbe_font_face_reference(mbe_font_face_t *face) {}
mbe_scaled_font_t *
mbe_scaled_font_create(mbe_font_face_t *face, co_aix fnt_mtx[6],
		       co_aix ctm[6]) {}
mbe_scaled_font_t *mbe_get_scaled_font(mbe_t *canvas) {}
void mbe_scaled_font_text_extents(mbe_scaled_font_t *scaled,
					 const char *txt,
					 mbe_text_extents_t *extents) {}

void mbe_font_face_destroy(mbe_font_face_t *face) {}
void mbe_paint_with_alpha(mbe_t *canvas, co_aix alpha) {}
void mbe_surface_destroy(mbe_surface_t *surface) {}
void mbe_set_source_rgba(mbe_t *canvas,
				co_aix r, co_aix g, co_aix b, co_aix a) {}
void mbe_set_scaled_font(mbe_t *canvas,
				const mbe_scaled_font_t *scaled) {}
void mbe_set_source_rgb(mbe_t *canvas, co_aix r, co_aix g, co_aix b) {}
void mbe_set_line_width(mbe_t *canvas, co_aix width) {}
mbe_font_face_t *mbe_get_font_face(mbe_t *canvas) {}
void mbe_fill_preserve(mbe_t *canvas) {}
void mbe_set_source(mbe_t *canvas, mbe_pattern_t *source) {}
void mbe_reset_clip(mbe_t *canvas) {}
mbe_surface_t *mbe_get_target(mbe_t *canvas) {}
void mbe_close_path(mbe_t *canvas) {}
void mbe_text_path(mbe_t *canvas, const char *txt) {}
void mbe_rectangle(mbe_t *canvas, co_aix x, co_aix y,
			  co_aix width, co_aix height) {}
int mbe_in_stroke(mbe_t *canvas, co_aix x, co_aix y) {}
void mbe_new_path(mbe_t *canvas) {}
void mbe_curve_to(mbe_t *canvas, co_aix x1, co_aix y1,
			 co_aix x2, co_aix y2,
			 co_aix x3, co_aix y3) {}
void mbe_restore(mbe_t *canvas) {}
void mbe_move_to(mbe_t *canvas, co_aix x, co_aix y) {}
void mbe_line_to(mbe_t *canvas, co_aix x, co_aix y) {}
int mbe_in_fill(mbe_t *canvas, co_aix x, co_aix y) {}
void mbe_stroke(mbe_t *canvas) {}

mbe_t *mbe_create(mbe_surface_t *target) {
    mbe_t *mbe;
    SkBitmap *bitmap = (SkBitmap *)target;

    mbe = (mbe_t *)malloc(sizeof(mbe_t));
    if(mbe == NULL)
	return NULL;
    
    mbe->canvas = new SkCanvas(*bitmap);
    if(mbe->canvas == NULL) {
	free(mbe);
	return NULL;
    }
    
    mbe->shader = NULL;
    mbe->shader_owned = 0;

    return mbe;
}

void mbe_destroy(mbe_t *canvas) {
    delete canvas->canvas;
    if(canvas->shader && canvas->shader_owned)
	delete canvas->shader;
    free(canvas);
}

void mbe_paint(mbe_t *canvas) {}
void mbe_save(mbe_t *canvas) {}
void mbe_fill(mbe_t *canvas) {}
void mbe_clip(mbe_t *canvas) {}

mbe_font_face_t * mbe_query_font_face(const char *family,
					     int slant, int weight) {}
void mbe_free_font_face(mbe_font_face_t *face) {}

void mbe_clear(mbe_t *canvas) {}
void mbe_copy_source(mbe_t *canvas) {}
void mbe_transform(mbe_t *mbe, co_aix matrix[6]) {}
void mbe_arc(mbe_t *mbe, co_aix x, co_aix y, co_aix radius,
		    co_aix angle_start, co_aix angle_stop) {}


C_END
