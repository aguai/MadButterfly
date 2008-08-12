changequote(`[', `]')dnl
divert([-1])

define([ADD_LINEAR_PAINT],[[
    paint_t *$1;
]])

define([ADD_RADIAL_PAINT],[[
    paint_t *$1;
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
define([COLOR_STOP],[ ])

define([REF_STOPS],)
define([FILL_SHAPE],[[
    paint_t *$1_fill;
]])
define([STROKE_SHAPE],[[
    paint_t *$1_stroke;
]])
define([FILL_SHAPE_WITH_PAINT],)
define([STROKE_SHAPE_WITH_PAINT],)

define([MADBUTTERFLY],[dnl
[#ifndef __$1_H_
#define __$1_H_]

[typedef struct $1 $1_t;]

struct [$1] {
    coord_t *root_coord;
$2[]dnl
};

[#endif /* __$1_H_ */]
])
divert[]dnl
