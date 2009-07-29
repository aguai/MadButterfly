changequote(`[', `]')dnl
divert([-1])

define([ADD_LINEAR_PAINT],[
[    paint_t *$1;
]])

define([ADD_RADIAL_PAINT],[
[    paint_t *$1;
]])
define([ADD_PATH],[
[    shape_t *$1;
]])
define([ADD_RECT],[
[    shape_t *$1;
]])
define([ADD_COORD],[
[    coord_t *$1;
]])
define([ADD_TEXT],[
[    shape_t *$1;
]])
define([ADD_STEXT],[
[    shape_t *$1;
     int $1_style_blks_num;
     mb_style_blk_t *$1_style_blks;
]])
define([ADD_IMAGE],[[
    paint_t *$1_paint_img;
    shape_t *$1;
]])
define([PANGO_BEGIN_TEXT],[
[    shape_t *$1;
]])
define([PANGO_END_TEXT],[])
define([PANGO_SIZE],[])
define([PANGO_STYLE],[])
define([PANGO_WEIGHT],[])
define([PANGO_FAMILY],[])
define([COLOR_STOP],[ ])

define([REF_STOPS_RADIAL],)
define([REF_STOPS_LINEAR],)
define([FILL_SHAPE],[
[    paint_t *$1_fill;
]])
define([STROKE_SHAPE],[
[    paint_t *$1_stroke;
]])
define([FILL_SHAPE_WITH_PAINT],)
define([STROKE_SHAPE_WITH_PAINT],)
define([STROKE_WIDTH],)
define([GROUP_HIDE],)
define([PATH_HIDE],)
define([RECT_HIDE],)
define([COORD_TRANSLATE],)
define([COORD_MATRIX],)
define([SHAPE_TRANSLATE],)
define([SHAPE_MATRIX],)
define([STYLE_BLOCK],[])
define([ADD_SYMBOL],)
define([SCENE])

define([MADBUTTERFLY],[dnl
[#ifndef __$1_H_
#define __$1_H_

typedef struct $1 $1_t;

struct $1 {
    mb_sprite_lsym_t lsym;
    redraw_man_t *rdman;
    const int *last_scene;
    coord_t *root_coord;]
$2[]dnl
[};

extern $1_t *$1_new(redraw_man_t *rdman, coord_t *parent_coord);
extern void $1_free($1_t *obj);

#endif /* __$1_H_ */]
])
divert[]dnl
