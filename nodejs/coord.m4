define([PROJ_PREFIX], [xnjsmb_auto_])dnl
STRUCT([coord], [coord_t], 
        [ACCESSOR([opacity], [xnjsmb_coord_get_opacity],[xnjsmb_coord_set_opacity])],
	[METHOD([add_shape], [xnjsmb_coord_add_shape],
		(SELF, OBJ([shape], [shape], [shape_t]), ERR), 1, []),
	 METHOD([remove], [xnjsmb_coord_remove], (SELF), 0, []),
	 METHOD([show], [xnjsmb_coord_show], (SELF), 0, []),
	 METHOD([hide], [xnjsmb_coord_hide], (SELF), 0, [])],
	((GET_INDEX, (coord_get_index, NUMBER)),
	 (SET_INDEX, (coord_set_index, NUMBER)),
	 ([STMOD], [xnjsmb_coord_mod])))
