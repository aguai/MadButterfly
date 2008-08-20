changequote(`[', `]')dnl
include([foreach.m4])dnl
divert([-1])

define([UNQUOTE], [$*])

define([QUOTE], [[[$*]]])

define([COUNT],[ifelse([$*],[],0,[$#])])

define([IMPORT],[define([$1],[$2$1(]$[]@[)])])

define([D_COLOR_STOP],[
	{$6,$2,$3,$4,$5}])

define([D_ADD_LINEAR_PAINT],[dnl
ifelse(COUNT($6),0,,[dnl
    static const int n_$1_stops = COUNT($6);
    static const grad_stop_t $1_stops[[]] = {UNQUOTE($6)};
])dnl
])

define([D_ADD_RADIAL_PAINT],[dnl
ifelse(COUNT($5),0,,[dnl
    static const int n_$1_stops = COUNT($5);
    static const grad_stop_t $1_stops[[]] = {UNQUOTE($5)};
])dnl
])

define([DECLARE_VARS], [divert([-1])
define([DIMPORT],[IMPORT(]QUOTE($[]1)[,[D_])])
DIMPORT([ADD_LINEAR_PAINT])
DIMPORT([ADD_RADIAL_PAINT])
DIMPORT([COLOR_STOP])
define([REF_STOPS_RADIAL])
define([REF_STOPS_LINEAR])
define([ADD_PATH])
define([ADD_RECT])
define([ADD_COORD])
define([FILL_SHAPE])
define([STROKE_SHAPE])
define([FILL_SHAPE_WITH_PAINT])
define([STROKE_SHAPE_WITH_PAINT])
divert[]])

define([S_ADD_LINEAR_PAINT],[
    obj->$1 = paint_linear_new(rdman, $2, $3, $4, $5);
ifelse(COUNT($6),0,,[dnl
    stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * n_$1_stops);
    memcpy(stops, $1_stops, sizeof(grad_stop_t) * n_$1_stops);
    paint_linear_stops(obj->$1, n_$1_stops, stops);
])dnl
])

define([S_ADD_RADIAL_PAINT],[
    obj->$1 = paint_radial_new(rdman, $2, $3, $4);
ifelse(COUNT($5),0,,[
    stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * n_$1_stops);
    memcpy(stops, $1_stops, sizeof(grad_stop_t) * n_$1_stops);
    paint_radial_stops(obj->$1, n_$1_stops, stops);
])dnl
])

define([S_COLOR_STOP],[])

define([S_REF_STOPS_RADIAL],[dnl
[    stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * n_$2_stops);
    memcpy(stops, $2_stops, sizeof(grad_stop_t) * n_$2_stops);
    paint_radial_stops(obj->$1, n_$2_stops, stops);
]])

define([S_REF_STOPS_LINEAR],[dnl
[    stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * n_$2_stops);
    memcpy(stops, $2_stops, sizeof(grad_stop_t) * n_$2_stops);
    paint_linear_stops(obj->$1, n_$2_stops, stops);
]])

define([S_ADD_RECT],[[
    obj->$1 = sh_rect_new($2, $3, $4, $5, 0, 0);
    rdman_add_shape(rdman, obj->$1, obj->$6);
]])

define([S_ADD_PATH],[[
    obj->$1 = sh_path_new("$2");
    rdman_add_shape(rdman, obj->$1, obj->$3);
]])

define([S_ADD_COORD],[[
    obj->$1 = rdman_coord_new(rdman, obj->$2);
]])

define([S_FILL_SHAPE_WITH_PAINT],[dnl
[    rdman_paint_fill(rdman, obj->$2, obj->$1);
]])

define([S_STROKE_SHAPE_WITH_PAINT],[dnl
[    rdman_paint_stroke(rdman, obj->$2, obj->$1);
]])

define([S_FILL_SHAPE],[dnl
[    obj->$1_fill = paint_color_new(rdman, $2, $3, $4, $5);
    rdman_paint_fill(rdman, obj->$1_fill, obj->$1);
]])

define([S_STROKE_SHAPE],[dnl
[    obj->$1_stroke = paint_color_new(rdman, $2, $3, $4, $5);
    rdman_paint_stroke(rdman, obj->$1_stroke, obj->$1);
]])

define([SETUP_VARS],[divert([-1])
define([SIMPORT],[IMPORT(]QUOTE($[]1)[,[S_])])
SIMPORT([ADD_LINEAR_PAINT])
SIMPORT([ADD_RADIAL_PAINT])
SIMPORT([COLOR_STOP])
SIMPORT([REF_STOPS_RADIAL])
SIMPORT([REF_STOPS_LINEAR])
SIMPORT([ADD_PATH],)
SIMPORT([ADD_RECT])
SIMPORT([ADD_COORD])
SIMPORT([FILL_SHAPE])
SIMPORT([STROKE_SHAPE])
SIMPORT([FILL_SHAPE_WITH_PAINT])
SIMPORT([STROKE_SHAPE_WITH_PAINT])
divert[]])

define([F_ADD_LINEAR_PAINT],[[
    stops = paint_linear_stops(obj->$1, 0, NULL);
    free(stops);
    obj->$1->free(obj->$1);
]])

define([F_ADD_RADIAL_PAINT],[[
    stops = paint_radial_stops(obj->$1, 0, NULL);
    free(stops);
    obj->$1->free(obj->$1);
]])

define([F_ADD_PATH],[[
    obj->$1->free(obj->$1);
]]);

define([F_ADD_RECT],[[
    obj->$1->free(obj->$1);
]]);

define([F_FILL_SHAPE],[[
    obj->$1_fill->free(obj->$1_fill);
]])

define([F_STROKE_SHAPE],[[
    obj->$1_stroke->free(obj->$1_stroke);
]])

define([CLEAR_VARS],[divert([-1])
define([FIMPORT],[IMPORT(]QUOTE($[]1)[,[F_])])
FIMPORT([ADD_LINEAR_PAINT])
FIMPORT([ADD_RADIAL_PAINT])
define([COLOR_STOP])
define([REF_STOPS_RADIAL])
define([REF_STOPS_LINEAR])
FIMPORT([ADD_PATH],)
FIMPORT([ADD_RECT])
define([ADD_COORD])
FIMPORT([FILL_SHAPE])
FIMPORT([STROKE_SHAPE])
define([FILL_SHAPE_WITH_PAINT])
define([STROKE_SHAPE_WITH_PAINT])
divert[]])

define([MADBUTTERFLY],[dnl
[#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mb_types.h"
#include "redraw_man.h"
#include "shapes.h"
#include "paint.h"
#include "$1.h"

$1_t *$1_new(redraw_man_t *rdman) {
    $1_t *obj;
    grad_stop_t *stops = NULL;]DECLARE_VARS
$2[]dnl
[
    obj = ($1_t *)malloc(sizeof($1_t));
    if(obj == NULL) return NULL;
]SETUP_VARS
    obj->root_coord = rdman->root_coord;
$2
[    return obj;
}

void $1_free($1_t *obj) {
    grad_stop_t *stops = NULL;
]CLEAR_VARS[]$2[
    free(obj);
}
]dnl
])
divert[]dnl
