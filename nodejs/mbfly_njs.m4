dnl
define([PROJ_PREFIX], [xnjsmb_auto_])dnl
dnl
STRUCT([mb_rt], [njs_runtime_t], [],
       [METHOD([coord_new], [xnjsmb_coord_new],
		 (OBJ([parent], [coord], [coord_t]), ERR), 1,
		 [OBJ([coord], [coord_t])],
		 (([MOD], [xnjsmb_mb_rt_objs_mod]))),
        METHOD([redraw_changed], [xnjsmb_redraw_changed], (), 0, []),
	METHOD([redraw_all], [xnjsmb_redraw_all], (), 0, []),
	METHOD([path_new], [xnjsmb_path_new], (STR(txt)), 1,
	       [OBJ([path], [shape_t])], (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([stext_new], [xnjsmb_stext_new],
	       (STR(txt), NUMBER(x), NUMBER(y)), 3,
	       [OBJ([stext], [shape_t])],
	       (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([image_new], [xnjsmb_image_new],
	       (NUMBER(x), NUMBER(y), NUMBER(w), NUMBER(h)), 4,
	       [OBJ([image], [shape_t])],
	       (([MOD], [xnjsmb_mb_rt_objs_mod])))],
	((CTOR, ([_X_njs_MB_new], (SELF, STR(display_name), INT(width), INT(height)), 3)))dnl
)
