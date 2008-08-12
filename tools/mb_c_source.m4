changequote(`[', `]')dnl
include([foreach.m4])dnl
divert([-1])

define([COUNT],[pushdef([COUNT_N])define([COUNT_N],0)dnl
foreach([x],[($1)],[define([COUNT_N],incr(COUNT_N))])COUNT_N[]dnl
popdef([COUNT_N])[]])

define([STOP_FIELDS],[dnl
ifelse(COUNT($2),0,,[dnl
[    int n_$1_stops;
    grad_stop_t *$1_stop;
]])dnl
])

define([IMPORT],[define([$1],[$2$1(][$][@)])])

define([D_COLOR_STOP],[{$2,$3,$4,$5,$6},])

define([D_ADD_LINEAR_PAINT],[dnl
ifelse(COUNT([$6]),0,,[dnl
    static int n_$1_stops = COUNT([$6]);
    static grad_stop_t $1_stops[[]] = {$6};
])dnl
])

define([D_ADD_RADIAL_PAINT],[dnl
ifelse(COUNT([$5]),0,,[dnl
    static int n_$1_stops = COUNT([$5]);
])dnl
])

define([D_REF_STOPS],[]);

define([DECLARE_VARS], [divert([-1])
IMPORT([ADD_LINEAR_PAINT], [D_])
IMPORT([ADD_RADIAL_PAINT],[D_])
IMPORT([REF_STOPS],[D_])
IMPORT([COLOR_STOP],[D_])
divert[]])

define([S_ADD_LINEAR_PAINT],[
    obj->$1 = mb_linear_new(rdman, $2, $3, $4, $5);
])
define([S_ADD_RADIAL_PAINT],[
    obj->$1 = mb_radial_new(rdman, $2, $3, $4);
])
define([S_REF_STOPS],[dnl
])

define([SETUP_VARS],[divert([-1])
IMPORT([ADD_LINEAR_PAINT],[S_])
IMPORT([ADD_RADIAL_PAINT],[S_])
IMPORT([REF_STOPS],[S_])
divert[]])

define([MADBUTTERFLY],[dnl
[#include <stdio.h>
#include "mb_types.h"
#include "redraw_man.h"
#include "shapes.h"
#include "paint.h"
#include "$1.h"

$1_t *$1_new(redraw_mant_t *rdman) {
    $1_t *obj;]DECLARE_VARS
$2[]dnl
[
    obj = ($1_t *)malloc(sizeof($1_t));
    if(obj == NULL) return NULL;
]SETUP_VARS
$2
[    return obj;
}]
])
divert[]dnl
