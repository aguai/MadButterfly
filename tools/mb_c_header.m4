changequote(`[', `]')dnl
define([ADD_LINEAR_PAINT],[[    paint_t *$1;
]])dnl
define([ADD_RADIAL_PAINT],[[    paint_t *$1;
]])dnl
define([ADD_PATH],[[    shape_t *$1;
]])dnl
define([ADD_RECT],[[    shape_t *$1;
]])dnl
define([ADD_COORD],[[    coord_t *$1;
]])dnl
dnl
define([REF_STOPS],)dnl
define([ADD_STOP],)dnl
define([FILL_SHAPE],)dnl
define([STROKE_SHAPE],)dnl
define([FILL_SHAPE_WITH_PAINT],)dnl
define([STROKE_SHAPE_WITH_PAINT],)dnl
dnl
define([MADBUTTERFLY],[dnl
[#ifndef __$1_H_
#define __$1_H_]

[typedef struct $1 $1_t;]

struct [$1] {
$2dnl
};

[#endif /* __$1_H_ */]
])dnl
dnl