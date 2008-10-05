#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mb_types.h"
#include "shapes.h"

typedef struct _sh_rect {
    shape_t shape;
    co_aix x, y;
    co_aix w, h;
    co_aix rx, ry;
    co_aix poses[12][2];
} sh_rect_t;

static void sh_rect_free(shape_t *shape) {
    free(shape);
}

shape_t *rdman_shape_rect_new(redraw_man_t *rdman,
			      co_aix x, co_aix y, co_aix w, co_aix h,
			      co_aix rx, co_aix ry) {
    sh_rect_t *rect;

    rect = (sh_rect_t *)malloc(sizeof(sh_rect_t));
    if(rect == NULL)
	return NULL;

    memset(rect, 0, sizeof(sh_rect_t));

    rect->shape.sh_type = SHT_RECT;
    rect->x = x;
    rect->y = y;
    rect->w = w;
    rect->h = h;
    rect->rx = rx;
    rect->ry = ry;
    rect->shape.free = sh_rect_free;

    rdman_shape_man(rdman, (shape_t *)rect);

    return (shape_t *)rect;
}

void sh_rect_set(shape_t *shape, co_aix x, co_aix y,
		 co_aix w, co_aix h, co_aix rx, co_aix ry) {
    sh_rect_t *rect = (sh_rect_t *)shape;

    rect->x = x;
    rect->y = y;
    rect->w = w;
    rect->h = h;
    rect->rx = rx;
    rect->ry = ry;
}

void sh_rect_transform(shape_t *shape) {
    sh_rect_t *rect = (sh_rect_t *)shape;
    co_aix x, y, w, h, rx, ry;
    co_aix (*poses)[2];
    co_aix width;
    area_t *area;
    int i;

    x = rect->x;
    y = rect->y;
    w = rect->w;
    h = rect->h;
    rx = rect->rx;
    ry = rect->ry;

    poses = rect->poses;

    if(rect->rx != 0 && rect->ry != 0) {
	poses[0][0] = x + w - rx;
	poses[0][1] = y;
	poses[1][0] = x + w;
	poses[1][1] = y;
	poses[2][0] = x + w;
	poses[2][1] = y + ry;
	
	poses[3][0] = x + w;
	poses[3][1] = y + h - ry;
	poses[4][0] = x + w;
	poses[4][1] = y + h;
	poses[5][0] = x + w - rx;
	poses[5][1] = y + h;
	
	poses[6][0] = x + rx;
	poses[6][1] = y + h;
	poses[7][0] = x;
	poses[7][1] = y + h;
	poses[8][0] = x;
	poses[8][1] = y + h - ry;
	
	poses[9][0] = x;
	poses[9][1] = y + ry;
	poses[10][0] = x;
	poses[10][1] = y;
	poses[11][0] = x + rx;
	poses[11][1] = y;

	for(i = 0; i < 12; i++)
	    coord_trans_pos(shape->coord, &poses[i][0], &poses[i][1]);

	geo_from_positions(shape->geo, 12, poses);
    } else {
	poses[0][0] = x;
	poses[0][1] = y;
	poses[1][0] = x + w;
	poses[1][1] = y;
	poses[2][0] = x + w;
	poses[2][1] = y + h;
	poses[3][0] = x;
	poses[3][1] = y + h;

	for(i = 0; i < 4; i++)
	    coord_trans_pos(shape->coord, &poses[i][0], &poses[i][1]);

	geo_from_positions(shape->geo, 4, poses);
    }

    if(shape->stroke) {
	area = shape->geo->cur_area;
	width = shape->stroke_width;
	area->x -= width / 2 + 1;
	area->y -= width / 2 + 1;
	area->w += width + 2;
	area->h += width + 2;
    }
}

void sh_rect_draw(shape_t *shape, cairo_t *cr) {
    sh_rect_t *rect = (sh_rect_t *)shape;
    int i;
    co_aix (*poses)[2];

    poses = rect->poses;
    if(rect->rx != 0 && rect->ry != 0) {
	cairo_move_to(cr, poses[11][0], poses[11][1]);
	for(i = 0; i < 12; i += 3) {
	    cairo_line_to(cr, poses[i][0], poses[i][1]);
	    cairo_curve_to(cr,
			   poses[i + 1][0], poses[i + 1][1],
			   poses[i + 1][0], poses[i + 1][1],
			   poses[i + 2][0], poses[i + 2][1]);
	}
    } else {
	cairo_move_to(cr, poses[3][0], poses[3][1]);
	for(i = 0; i < 4; i++)
	    cairo_line_to(cr, poses[i][0], poses[i][1]);
    }
}
