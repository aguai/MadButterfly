define([PROJ_PREFIX], [xnjsmb_auto_])dnl
dnl
STRUCT([paint], [paint_t], [],
       [METHOD([fill], [xnjsmb_paint_fill],
               (SELF, OBJ([sh], [shape], [shape_t])), 1, []),
        METHOD([stroke], [xnjsmb_paint_stroke],
	       (SELF, OBJ([sh], [shape], [shape_t])), 1, [])])

STRUCT([paint_color], [paint_t], [],
       [METHOD([set_color], [xnjsmb_paint_color_set_color],
       	       (SELF, NUMBER([r]), NUMBER([g]), NUMBER([b]), NUMBER([a])),
	       4, [])],
       (([INHERIT], [paint])))

STRUCT([paint_image], [paint_t], [],
       [],
       (([INHERIT], [paint])))

STRUCT([paint_linear], [paint_t], [],
       [METHOD([set_stops], [xnjsmb_paint_linear_set_stops],
       (ARRAY([stops])), 1, [])],
       (([INHERIT], [paint])))

STRUCT([paint_radial], [paint_t], [],
       [METHOD([set_stops], [xnjsmb_paint_radial_set_stops],
       (ARRAY([stops])), 1, [])],
       (([INHERIT], [paint])))
