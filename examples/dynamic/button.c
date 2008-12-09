#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mb_types.h>
#include <mb_redraw_man.h>
#include <mb_shapes.h>
#include <mb_paint.h>
#include "button.h"

button_t *button_new(redraw_man_t *rdman, coord_t *parent_coord) {
    button_t *obj;
    grad_stop_t *stops = NULL;
    static const int n_linearGradient4066_stops = 2;
    static const grad_stop_t linearGradient4066_stops[] = {
	{0.000000,0.000000,0.000000,0.000000,0.000000},
	{1.000000,0.000000,0.000000,0.000000,1.000000}};
    static const int n_linearGradient4026_stops = 2;
    static const grad_stop_t linearGradient4026_stops[] = {
	{0.000000,0.000000,0.000000,0.000000,1.000000},
	{1.000000,1.000000,1.000000,1.000000,0.000000}};
    static const int n_linearGradient4018_stops = 2;
    static const grad_stop_t linearGradient4018_stops[] = {
	{0.000000,0.000000,0.000000,0.000000,1.000000},
	{1.000000,0.000000,0.000000,1.000000,0.000000}};

    obj = (button_t *)malloc(sizeof(button_t));
    if(obj == NULL) return NULL;
    obj->rdman = rdman;

    obj->root_coord = rdman_coord_new(rdman, parent_coord);

    obj->linearGradient4066 = rdman_paint_linear_new(rdman, 0.000000, 0.000000, 0.000000, 0.000000);
    stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * n_linearGradient4066_stops);
    memcpy(stops, linearGradient4066_stops, sizeof(grad_stop_t) * n_linearGradient4066_stops);
    paint_linear_stops(obj->linearGradient4066, n_linearGradient4066_stops, stops);

    obj->linearGradient4026 = rdman_paint_linear_new(rdman, 0.000000, 0.000000, 0.000000, 0.000000);
    stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * n_linearGradient4026_stops);
    memcpy(stops, linearGradient4026_stops, sizeof(grad_stop_t) * n_linearGradient4026_stops);
    paint_linear_stops(obj->linearGradient4026, n_linearGradient4026_stops, stops);

    obj->linearGradient4018 = rdman_paint_linear_new(rdman, 0.000000, 0.000000, 0.000000, 0.000000);
    stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * n_linearGradient4018_stops);
    memcpy(stops, linearGradient4018_stops, sizeof(grad_stop_t) * n_linearGradient4018_stops);
    paint_linear_stops(obj->linearGradient4018, n_linearGradient4018_stops, stops);

    obj->layer1 = rdman_coord_new(rdman, obj->root_coord);

    obj->normal = rdman_coord_new(rdman, obj->layer1);
    memset(obj->normal->matrix, 0, sizeof(obj->normal->matrix));
    obj->normal->matrix[0] = 1;
    obj->normal->matrix[2] = 460.000000;
    obj->normal->matrix[4] = 1;
    obj->normal->matrix[5] = 87.142857;
    rdman_coord_changed(rdman, obj->normal);

    obj->rect6048_coord = rdman_coord_new(rdman, obj->normal);
    obj->rect6048_coord->matrix[0] = 0.374016;
    obj->rect6048_coord->matrix[3] = 0.000000;
    obj->rect6048_coord->matrix[1] = 0.000000;
    obj->rect6048_coord->matrix[4] = 0.534091;
    obj->rect6048_coord->matrix[2] = -66.074260;
    obj->rect6048_coord->matrix[5] = -121.975700;
    rdman_coord_changed(rdman, obj->rect6048_coord);

    obj->rect6048 = rdman_shape_rect_new(rdman, 181.428570, 235.933610, 360.000000, 118.571430, 16.000000, 16.000000);

    rdman_add_shape(rdman, obj->rect6048, obj->rect6048_coord);
    obj->rect6048_fill = rdman_paint_color_new(rdman, 0.000000, 0.560784, 1.000000, 1.000000);
    rdman_paint_fill(rdman, obj->rect6048_fill, obj->rect6048);

    obj->rect6050 = rdman_shape_rect_new(rdman, 1.248565, 0.600819, 134.645690, 63.327908, 5.984253, 8.545453);

    rdman_add_shape(rdman, obj->rect6050, obj->normal);
    obj->rect6050_fill = rdman_paint_color_new(rdman, 0.000000, 0.560784, 1.000000, 1.000000);
    rdman_paint_fill(rdman, obj->rect6050_fill, obj->rect6050);

    obj->rect6052 = rdman_shape_rect_new(rdman, 0.714294, 0.219334, 134.645690, 63.327908, 5.984253, 8.545453);

    rdman_add_shape(rdman, obj->rect6052, obj->normal);
    obj->rect6052_fill = rdman_paint_color_new(rdman, 0.000000, 0.560784, 1.000000, 1.000000);
    rdman_paint_fill(rdman, obj->rect6052_fill, obj->rect6052);

    obj->tspan6056 = rdman_shape_text_new(rdman, "Line", 26.428562, 48.076523, 40.000000,
    	      				  cairo_get_font_face(rdman->cr));
    rdman_add_shape(rdman, obj->tspan6056, obj->normal);
    obj->tspan6056_fill = rdman_paint_color_new(rdman, 1.000000, 1.000000, 1.000000, 1.000000);
    rdman_paint_fill(rdman, obj->tspan6056_fill, obj->tspan6056);
    obj->tspan6056->stroke_width = 1.000000;

    obj->active = rdman_coord_new(rdman, obj->layer1);
    memset(obj->active->matrix, 0, sizeof(obj->active->matrix));
    obj->active->matrix[0] = 1;
    obj->active->matrix[2] = 102.857140;
    obj->active->matrix[4] = 1;
    obj->active->matrix[5] = 85.714284;
    rdman_coord_changed(rdman, obj->active);

    obj->rect4074_coord = rdman_coord_new(rdman, obj->active);
    obj->rect4074_coord->matrix[0] = 0.374016;
    obj->rect4074_coord->matrix[3] = 0.000000;
    obj->rect4074_coord->matrix[1] = 0.000000;
    obj->rect4074_coord->matrix[4] = 0.534091;
    obj->rect4074_coord->matrix[2] = -66.788541;
    obj->rect4074_coord->matrix[5] = -122.689990;
    rdman_coord_changed(rdman, obj->rect4074_coord);

    obj->rect4074 = rdman_shape_rect_new(rdman, 181.428570, 235.933610, 360.000000, 118.571430, 16.000000, 16.000000);

    rdman_add_shape(rdman, obj->rect4074, obj->rect4074_coord);
    obj->rect4074_fill = rdman_paint_color_new(rdman, 0.000000, 0.000000, 1.000000, 1.000000);
    rdman_paint_fill(rdman, obj->rect4074_fill, obj->rect4074);

    obj->rect3240 = rdman_shape_rect_new(rdman, 0.534297, -0.113462, 134.645690, 63.327908, 5.984253, 8.545453);

    rdman_add_shape(rdman, obj->rect3240, obj->active);
    obj->rect3240_fill = rdman_paint_color_new(rdman, 0.000000, 0.000000, 1.000000, 1.000000);
    rdman_paint_fill(rdman, obj->rect3240_fill, obj->rect3240);

    obj->rect3242 = rdman_shape_rect_new(rdman, -0.000000, -0.494952, 134.645690, 63.327908, 5.984253, 8.545453);

    rdman_add_shape(rdman, obj->rect3242, obj->active);
    obj->rect3242_fill = rdman_paint_color_new(rdman, 0.000000, 0.000000, 1.000000, 1.000000);
    rdman_paint_fill(rdman, obj->rect3242_fill, obj->rect3242);

    obj->tspan4387 = rdman_shape_text_new(rdman, "Line", 25.714287, 47.362209, 40.000000,
    	      				  cairo_get_font_face(rdman->cr));
    rdman_add_shape(rdman, obj->tspan4387, obj->active);
    obj->tspan4387_fill = rdman_paint_color_new(rdman, 1.000000, 1.000000, 1.000000, 1.000000);
    rdman_paint_fill(rdman, obj->tspan4387_fill, obj->tspan4387);
    obj->tspan4387->stroke_width = 1.000000;

    obj->layer2 = rdman_coord_new(rdman, obj->root_coord);

    return obj;
}

void button_free(button_t *obj) {
    grad_stop_t *stops = NULL;
    redraw_man_t *rdman;

    rdman = obj->rdman;

    rdman_paint_free(rdman, obj->tspan4387_fill);

    rdman_shape_free(rdman, obj->tspan4387);

    rdman_paint_free(rdman, obj->rect3242_fill);

    rdman_shape_free(rdman, obj->rect3242);

    rdman_paint_free(rdman, obj->rect3240_fill);

    rdman_shape_free(rdman, obj->rect3240);

    rdman_paint_free(rdman, obj->rect4074_fill);

    rdman_shape_free(rdman, obj->rect4074);

    rdman_paint_free(rdman, obj->tspan6056_fill);

    rdman_shape_free(rdman, obj->tspan6056);

    rdman_paint_free(rdman, obj->rect6052_fill);

    rdman_shape_free(rdman, obj->rect6052);

    rdman_paint_free(rdman, obj->rect6050_fill);

    rdman_shape_free(rdman, obj->rect6050);

    rdman_paint_free(rdman, obj->rect6048_fill);

    rdman_shape_free(rdman, obj->rect6048);

    stops = paint_linear_stops(obj->linearGradient4018, 0, NULL);
    free(stops);
    rdman_paint_free(rdman, obj->linearGradient4018);

    stops = paint_linear_stops(obj->linearGradient4026, 0, NULL);
    free(stops);
    rdman_paint_free(rdman, obj->linearGradient4026);

    stops = paint_linear_stops(obj->linearGradient4066, 0, NULL);
    free(stops);
    rdman_paint_free(rdman, obj->linearGradient4066);

    rdman_coord_subtree_free(rdman, obj->root_coord);
    free(obj);
}
