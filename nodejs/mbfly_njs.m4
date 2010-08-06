dnl
define([PROJ_PREFIX], [xnjsmb_auto_])dnl
dnl
STRUCT([mb_rt], [njs_runtime_t], [],
       [METHOD([coord_new], [xnjsmb_coord_new],
		 (OBJ([parent], [coord], [coord_t]), ERR), 1,
		 [OBJ([coord], [coord_t])], (([MOD], xnjsmb_coord_new_mod))),
        METHOD([redraw_changed], [xnjsmb_redraw_changed], (), 0, []),
	METHOD([redraw_all], [xnjsmb_redraw_all], (), 0, [])],
	((CTOR, ([_X_njs_MB_new], (SELF, STR(display_name), INT(width), INT(height)), 3)))dnl
)
