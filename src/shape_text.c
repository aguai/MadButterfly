#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cairo.h>
#include "mb_types.h"
#include "shapes.h"

#define OK 0
#define ERR -1

typedef struct _sh_text {
    shape_t shape;
    char *data;
    co_aix x, y;
    co_aix d_x, d_y;		/* device x and y */
    co_aix font_size;
    co_aix d_font_size;
    cairo_font_face_t *face;
    cairo_scaled_font_t *scaled_font;
    int flags;
} sh_text_t;

#define TXF_SCALE_DIRTY 0x1

shape_t *sh_text_new(const char *txt, co_aix x, co_aix y,
		     co_aix font_size, cairo_font_face_t *face) {
    sh_text_t *text;

    text = (sh_text_t *)malloc(sizeof(sh_text_t));
    if(text == NULL)
	return NULL;

    memset(text, 0, sizeof(sh_text_t));
    text->shape.sh_type = SHT_TEXT;
    text->data = strdup(txt);
    if(text->data == NULL) {
	free(text);
	return NULL;
    }
    text->x = x;
    text->y = y;
    text->font_size = font_size;
    cairo_font_face_reference(face);
    text->face = face;
    text->flags |= TXF_SCALE_DIRTY;

    return (shape_t *)text;
}

void sh_text_free(shape_t *shape) {
    sh_text_t *text = (sh_text_t *)shape;

    if(text->scaled_font)
	cairo_scaled_font_destroy(text->scaled_font);
    cairo_font_face_destroy(text->face);
}

static int get_extents(sh_text_t *text, cairo_text_extents_t *extents) {
    cairo_matrix_t fmatrix;
    cairo_matrix_t ctm;
    cairo_scaled_font_t *new_scaled;
    cairo_font_options_t *fopt;

    if((text->flags & TXF_SCALE_DIRTY) ||
       text->scaled_font == NULL) {
	fopt = cairo_font_options_create();
	if(fopt == NULL)
	    return ERR;
	memset(&fmatrix, 0, sizeof(cairo_matrix_t));
	fmatrix.xx = text->d_font_size;
	fmatrix.yy = text->d_font_size;
	memset(&ctm, 0, sizeof(cairo_matrix_t));
	ctm.xx = 1;
	ctm.yy = 1;
	new_scaled = cairo_scaled_font_create(text->face,
					      &fmatrix,
					      &ctm,
					      fopt);
	cairo_font_options_destroy(fopt);
	if(new_scaled == NULL)
	    return ERR;

	if(text->scaled_font)
	    cairo_scaled_font_destroy(text->scaled_font);
	text->scaled_font = new_scaled;
	text->flags &= ~TXF_SCALE_DIRTY;
    }

    cairo_scaled_font_text_extents(text->scaled_font,
				   text->data, extents);
    return OK;
}

void sh_text_transform(shape_t *shape) {
    sh_text_t *text;
    co_aix x, y;
    co_aix shw;
    cairo_text_extents_t extents;
    co_aix poses[2][2];
    int r;

    text = (sh_text_t *)shape;

    text->d_font_size = coord_trans_size(shape->coord, text->font_size);

    x = text->x;
    y = text->y;
    coord_trans_pos(shape->coord, &x, &y);
    r = get_extents(text, &extents);
    if(r != OK)
	/* TODO: announce error. change return type? */
	return;

    text->d_x = x;
    text->d_y = y;
    shw = shape->stroke_width / 2;
    /* FIXME: It is unreasonable that a font exceed it's bbox.
     * We add 5 pixels in get right bbox.  But, it is unreasonable.
     */
    poses[0][0] = x + extents.x_bearing - 5 - shw;
    poses[0][1] = y + extents.y_bearing - 5 - shw;
    poses[1][0] = poses[0][0] + extents.width + 10 + shape->stroke_width;
    poses[1][1] = poses[0][1] + extents.height + 10 + shape->stroke_width;
    geo_from_positions(shape->geo, 2, poses);
}


static void draw_text(sh_text_t *text, cairo_t *cr) {
    cairo_set_scaled_font(cr, text->scaled_font);
    cairo_move_to(cr, text->d_x, text->d_y);
    cairo_text_path(cr, text->data);
}


void sh_text_draw(shape_t *shape, cairo_t *cr) {
    draw_text((sh_text_t *)shape, cr);
}

