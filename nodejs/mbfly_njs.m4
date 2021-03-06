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
	METHOD([flush], [njs_mb_flush], (), 0, []),
	METHOD([path_new], [xnjsmb_path_new], (STR(txt)), 1,
	       [OBJ([path], [shape_t])], (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([stext_new], [xnjsmb_stext_new],
	       (STR(txt), NUMBER(x), NUMBER(y)), 3,
	       [OBJ([stext], [shape_t])],
	       (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([image_new], [xnjsmb_image_new],
	       (NUMBER(x), NUMBER(y), NUMBER(w), NUMBER(h)), 4,
	       [OBJ([image], [shape_t])],
	       (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([rect_new], [xnjsmb_rect_new],
	       (NUMBER(x), NUMBER(y), NUMBER(w), NUMBER(h),
	        NUMBER(rx), NUMBER(ry), ERR), 6,
	       [OBJ([rect], [shape_t])],
	       (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([paint_color_new], [xnjsmb_paint_color_new],
	       (NUMBER(r), NUMBER(g), NUMBER(b), NUMBER(a), ERR), 4,
	       [OBJ([paint_color], [paint_t])],
	       (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([paint_image_new], [xnjsmb_paint_image_new],
	       (OBJ([img], [img_data], [mb_img_data_t]), ERR), 1,
	       [OBJ([paint_image], [paint_t])],
	       (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([paint_linear_new], [xnjsmb_paint_linear_new],
	       (NUMBER(x1), NUMBER(y1), NUMBER(x2), NUMBER(y2), ERR), 4,
	       [OBJ([paint_linear], [paint_t])],
	       (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([paint_radial_new], [xnjsmb_paint_radial_new],
	       (NUMBER(cx), NUMBER(cy), NUMBER(r), ERR), 3,
	       [OBJ([paint_radial], [paint_t])],
	       (([MOD], [xnjsmb_mb_rt_objs_mod]))),
	METHOD([handle_single_event], [xnjsmb_handle_single_event],
	       (OBJ([evt], [event], [void])), 1, []),
	METHOD([no_more_event], [xnjsmb_no_more_event],
	       (), 0, [])],
	((CTOR, ([_njs_mb_new], (SELF, STR(display_name), INT(width), INT(height)), 3)))dnl
)
dnl
dnl
dnl
STRUCT([mb_rt_display], [void], [],
       [],
       ())dnl
dnl
dnl Function to create mb_rt for an existed window.
dnl
STRUCT([mb_rt_with_win], [njs_runtime_t], [],
       [],
       ((CTOR, ([_njs_mb_new_with_win],dnl
       	        (SELF, OBJ([display], [mb_rt_display], [void]),dnl
		 INT([window])),dnl
		 2)),dnl
        ([INHERIT], [mb_rt]))dnl
)
