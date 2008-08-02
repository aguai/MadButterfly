#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include "paint.h"

typedef struct _paint_color {
    paint_t paint;
    co_comp_t r, g, b;
    redraw_man_t *rdman;
} paint_color_t;


static void paint_color_prepare(paint_t *paint, cairo_t *cr) {
    paint_color_t *color = (paint_color_t *)paint;

    cairo_set_source_rgb(cr, color->r, color->g, color->b);
}

static void paint_color_free(paint_t *paint) {
    paint_color_t *color = (paint_color_t *)paint;

    shnode_list_free(color->rdman, paint->members);
    free(paint);
}

paint_t *paint_color_new(redraw_man_t *rdman,
			 co_comp_t r, co_comp_t g, co_comp_t b) {
    paint_color_t *color;

    color = (paint_color_t *)malloc(sizeof(paint_color_t));
    if(color == NULL)
	return NULL;
    color->rdman = rdman;
    color->r = r;
    color->g = g;
    color->b = b;
    paint_init(&color->paint, paint_color_prepare, paint_color_free);
    return (paint_t *)color;
}

void paint_color_set(paint_t *paint,
		     co_comp_t r, co_comp_t g, co_comp_t b) {
    paint_color_t *color = (paint_color_t *)paint;

    color->r = r;
    color->g = g;
    color->b = b;
}
