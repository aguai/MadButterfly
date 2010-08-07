define([PROJ_PREFIX], [xnjsmb_auto_])dnl
dnl
STRUCT([shape], [shape_t],
       [ACCESSOR([stroke_width],
		 [xnjsmb_shape_stroke_width_get],
		 [xnjsmb_shape_stroke_width_set])],
       [METHOD([show], [sh_show], (), 0, []),
        METHOD([hide], [sh_hide], (), 0, [])])

STRUCT([path], [shape_t], [], [], (([INHERIT], [shape])))

STRUCT([stext], [shape_t], [],
       [METHOD([set_text], [sh_stext_set_text], (STR([txt])), 1, []),
        METHOD([set_style], [xnjsmb_sh_stext_set_style],
	       (ARRAY([blks]), ERR), 1, [])],
       (([INHERIT], [shape])))

STRUCT([image], [shape_t], [], [], (([INHERIT], [shape])))
