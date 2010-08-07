define([PROJ_PREFIX], [xnjsmb_auto_])dnl
dnl
STRUCT([paint], [paint_t], [],
       [METHOD([fill], [xnjsmb_paint_fill],
               (SELF, OBJ([sh], [shape], [shape_t])), 1, []),
        METHOD([stroke], [xnjsmb_paint_stroke],
	       (SELF, OBJ([sh], [shape], [shape_t])), 1, [])])

STRUCT([paint_color], [paint_t], [],
       [],
       (([INHERIT], [paint])))

STRUCT([paint_image], [paint_t], [],
       [],
       (([INHERIT], [paint])))
