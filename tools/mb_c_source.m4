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
define([ADD_TEXT],)
define([FILL_SHAPE])
define([STROKE_SHAPE])
define([FILL_SHAPE_WITH_PAINT])
define([STROKE_SHAPE_WITH_PAINT])
define([STROKE_WIDTH])
define([GROUP_HIDE],)
define([RECT_HIDE],)
define([PATH_HIDE],)
define([COORD_TRANSLATE],)
define([COORD_MATRIX],)
define([SHAPE_TRANSLATE],)
define([SHAPE_MATRIX],)
define([ADD_SYMBOL],)
divert[]])

define([S_ADD_LINEAR_PAINT],[
    obj->$1 = rdman_paint_linear_new(rdman, $2, $3, $4, $5);
ifelse(COUNT($6),0,,[dnl
    stops = (grad_stop_t *)malloc(sizeof(grad_stop_t) * n_$1_stops);
    memcpy(stops, $1_stops, sizeof(grad_stop_t) * n_$1_stops);
    paint_linear_stops(obj->$1, n_$1_stops, stops);
])dnl
])

define([S_ADD_RADIAL_PAINT],[
    obj->$1 = rdman_paint_radial_new(rdman, $2, $3, $4);
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
    obj->$1 = rdman_shape_rect_new(rdman, $2, $3, $4, $5, $6, $7);

    rdman_add_shape(rdman, obj->$1, obj->$8);
]])

define([S_ADD_PATH],[[
    {
        char _cmds[] = "$3";
        float _args[] = {$4};
        int _fix_args[] = {$6};
        obj->$1 = rdman_shape_path_new_from_binary(rdman, _cmds,_args,$5,_fix_args,$7);
        rdman_add_shape(rdman, obj->$1, obj->$2);
    }
]])

define([S_ADD_COORD],[[
    obj->$1 = rdman_coord_new(rdman, obj->$2);
]])

define([S_ADD_TEXT],[[
    obj->$1 = rdman_shape_text_new(rdman, "$2", $3, $4, $5,
    	      				  cairo_get_font_face(rdman->cr));
    rdman_add_shape(rdman, obj->$1, obj->$6);
]])

define([S_FILL_SHAPE_WITH_PAINT],[dnl
[    rdman_paint_fill(rdman, obj->$2, obj->$1);
]])

define([S_STROKE_SHAPE_WITH_PAINT],[dnl
[    rdman_paint_stroke(rdman, obj->$2, obj->$1);
]])

define([S_FILL_SHAPE],[dnl
[    obj->$1_fill = rdman_paint_color_new(rdman, $2, $3, $4, $5);
    rdman_paint_fill(rdman, obj->$1_fill, obj->$1);
]])

define([S_STROKE_SHAPE],[dnl
[    obj->$1_stroke = rdman_paint_color_new(rdman, $2, $3, $4, $5);
    rdman_paint_stroke(rdman, obj->$1_stroke, obj->$1);
]])

define([S_STROKE_WIDTH],[dnl
[    obj->$1->stroke_width = $2;
]])

define([S_GROUP_HIDE],[dnl
[    coord_hide(obj->$1);
]])

define([S_RECT_HIDE],[dnl
[    sh_hide(obj->$1);
]])

define([S_PATH_HIDE],[dnl
[    sh_hide(obj->$1);
]])

define([S_COORD_TRANSLATE],[dnl
[    memset(obj->$1->matrix, 0, sizeof(obj->$1->matrix));
    obj->$1->matrix[0] = 1;
    obj->$1->matrix[2] = $2;
    obj->$1->matrix[4] = 1;
    obj->$1->matrix[5] = $3;
    rdman_coord_changed(rdman, obj->$1);
]])

define([S_COORD_MATRIX],[dnl
[    obj->$1->matrix[0] = $2;
    obj->$1->matrix[3] = $3;
    obj->$1->matrix[1] = $4;
    obj->$1->matrix[4] = $5;
    obj->$1->matrix[2] = $6;
    obj->$1->matrix[5] = $7;
    rdman_coord_changed(rdman, obj->$1);
]])

define([S_SHAPE_TRANSLATE],[dnl
[    memset(obj->$1->matrix, 0, sizeof(obj->$1->matrix));
    obj->$1->matrix[0] = 1;
    obj->$1->matrix[2] = $2;
    obj->$1->matrix[4] = 1;
    obj->$1->matrix[5] = $3;
    rdman_coord_changed(rdman, obj->$1);
]])

define([S_SHAPE_MATRIX],[dnl
[    obj->$1->matrix[0] = $2;
    obj->$1->matrix[3] = $3;
    obj->$1->matrix[1] = $4;
    obj->$1->matrix[4] = $5;
    obj->$1->matrix[2] = $6;
    obj->$1->matrix[5] = $7;
    rdman_coord_changed(rdman, obj->$1);
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
SIMPORT([ADD_TEXT])
SIMPORT([FILL_SHAPE])
SIMPORT([STROKE_SHAPE])
SIMPORT([FILL_SHAPE_WITH_PAINT])
SIMPORT([STROKE_SHAPE_WITH_PAINT])
SIMPORT([STROKE_WIDTH])
SIMPORT([GROUP_HIDE])
SIMPORT([RECT_HIDE])
SIMPORT([PATH_HIDE])
SIMPORT([COORD_TRANSLATE])
SIMPORT([COORD_MATRIX])
SIMPORT([SHAPE_TRANSLATE])
SIMPORT([SHAPE_MATRIX])
define([ADD_SYMBOL],)
divert[]])

define([F_ADD_LINEAR_PAINT],[[
    stops = paint_linear_stops(obj->$1, 0, NULL);
    free(stops);
    rdman_paint_free(rdman, obj->$1);
]])

define([F_ADD_RADIAL_PAINT],[[
    stops = paint_radial_stops(obj->$1, 0, NULL);
    free(stops);
    rdman_paint_free(rdman, obj->$1);
]])

define([F_ADD_PATH],[[
    rdman_shape_free(rdman, obj->$1);
]])

define([F_ADD_RECT],[[
    rdman_shape_free(rdman, obj->$1);
]])

define([F_ADD_TEXT],[[
    rdman_shape_free(rdman, obj->$1);
]])

define([F_FILL_SHAPE],[[
    rdman_paint_free(rdman, obj->$1_fill);
]])

define([F_STROKE_SHAPE],[[
    rdman_paint_free(rdman, obj->$1_stroke);
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
FIMPORT([ADD_TEXT])
FIMPORT([FILL_SHAPE])
FIMPORT([STROKE_SHAPE])
define([FILL_SHAPE_WITH_PAINT])
define([STROKE_SHAPE_WITH_PAINT])
define([STROKE_WIDTH])
define([GROUP_HIDE],)
define([RECT_HIDE],)
define([PATH_HIDE],)
define([COORD_TRANSLATE],)
define([COORD_MATRIX],)
define([SHAPE_TRANSLATE],)
define([SHAPE_MATRIX],)
define([ADD_SYMBOL],)
divert[]])

define([REVERSE_VARS],[divert([-1])
define([__REV_VAR],[])
define([PUSH_REV], [
	pushdef([__REV_VAR])
	define([__REV_VAR], ]QUOTE(QUOTE($[]1))[)])
define([POP_ALL_REV], [dnl
ifelse(__REV_VAR, [], ,[UNQUOTE(__REV_VAR)[]dnl
popdef([__REV_VAR])[]POP_ALL_REV[]])])
define([RIMPORT], [
	define(]QUOTE($[]1)[,
		[PUSH_REV(]]QUOTE(QUOTE($[]1))[[(]QUOTE($[]@)[))])
])
RIMPORT([ADD_LINEAR_PAINT])
RIMPORT([ADD_RADIAL_PAINT])
RIMPORT([COLOR_STOP])
RIMPORT([REF_STOPS_RADIAL])
RIMPORT([REF_STOPS_LINEAR])
RIMPORT([ADD_PATH])
RIMPORT([ADD_RECT])
RIMPORT([ADD_COORD])
RIMPORT([ADD_TEXT])
RIMPORT([FILL_SHAPE])
RIMPORT([STROKE_SHAPE])
RIMPORT([FILL_SHAPE_WITH_PAINT])
RIMPORT([STROKE_SHAPE_WITH_PAINT])
RIMPORT([STROKE_WIDTH])
RIMPORT([GROUP_HIDE])
RIMPORT([RECT_HIDE])
RIMPORT([PATH_HIDE])
RIMPORT([COORD_TRANSLATE])
RIMPORT([COORD_MATRIX])
RIMPORT([SHAPE_TRANSLATE])
RIMPORT([SHAPE_MATRIX])
define([ADD_SYMBOL],)
divert[]dnl
])

define([Y_ADD_SYMBOL],[[{"$1", MB_SPRITE_OFFSET($1)},]])

define([DECLARE_SYMS], [divert([-1])
define([YIMPORT],[IMPORT(]QUOTE($[]1)[,[Y_])])
define([ADD_LINEAR_PAINT])
define([ADD_RADIAL_PAINT])
define([COLOR_STOP])
define([REF_STOPS_RADIAL])
define([REF_STOPS_LINEAR])
define([ADD_PATH])
define([ADD_RECT])
define([ADD_COORD])
define([ADD_TEXT],)
define([FILL_SHAPE])
define([STROKE_SHAPE])
define([FILL_SHAPE_WITH_PAINT])
define([STROKE_SHAPE_WITH_PAINT])
define([STROKE_WIDTH])
define([GROUP_HIDE],)
define([RECT_HIDE],)
define([PATH_HIDE],)
define([COORD_TRANSLATE],)
define([COORD_MATRIX],)
define([SHAPE_TRANSLATE],)
define([SHAPE_MATRIX],)
YIMPORT([ADD_SYMBOL])
divert[]dnl
])

define([MADBUTTERFLY],[dnl
[#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mb_types.h>
#include <mb_redraw_man.h>
#include <mb_shapes.h>
#include <mb_paint.h>
#include "$1.h"

#ifdef MB_SPRITE_OFFSET
#undef MB_SPRITE_OFFSET
#endif
#define MB_SPRITE_OFFSET(x) ((int)&((($1_t *)0)->x))

static
mb_sprite_lsym_entry_t $1_symbols[] = {]DECLARE_SYMS
$2[
};

#ifndef MB_LSYM_GET_OBJ_WITH_NAME
#define MB_LSYM_GET_OBJ_WITH_NAME
static
mb_obj_t *mb_lsym_get_obj_with_name(mb_sprite_lsym_t *lsym, const char *sym) {
    int i;

    for(i = 0; i < lsym->num_entries; i++) {
	if(strcmp(lsym->entries[i].sym, sym) != 0)
	    continue;
	return (mb_obj_t *)(((void *)lsym) + lsym->entries[i].offset);
    }
    return NULL;
}
#endif /* MB_LSYM_GET_OBJ_WITH_NAME */

void $1_free($1_t *);

$1_t *$1_new(redraw_man_t *rdman, coord_t *parent_coord) {
    $1_t *obj;
    grad_stop_t *stops = NULL;]DECLARE_VARS
$2[]dnl
[
    obj = ($1_t *)malloc(sizeof($1_t));
    if(obj == NULL) return NULL;

    obj->lsym.sprite.free = (void (*)(mb_sprite_t))$1_free;
    obj->lsym.sprite.get_obj_with_name =
	(mb_obj_t *(*)(mb_sprite_t *, const char *))mb_lsym_get_obj_with_name;
    obj->lsym.num_entries =
	sizeof($1_symbols) / sizeof(mb_sprite_lsym_entry_t);
    obj->lsym.entries = $1_symbols;

    obj->rdman = rdman;
]SETUP_VARS[
    obj->root_coord = rdman_coord_new(rdman, parent_coord);]
$2
[    return obj;
}

void $1_free($1_t *obj) {
    grad_stop_t *stops = NULL;
    redraw_man_t *rdman;

    rdman = obj->rdman;
]REVERSE_VARS[]dnl
divert([-1])dnl
$2[]dnl
divert[]dnl
CLEAR_VARS[]dnl
POP_ALL_REV[
    rdman_coord_subtree_free(rdman, obj->root_coord);
    free(obj);
}
]dnl
])
divert[]dnl
