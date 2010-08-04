define([PROJ_PREFIX], [xnjsmb_auto_])dnl
STRUCT([coord], [coord_t], [],
	[METHOD([add_shape], [xnjsmb_coord_add_shape],
		(SELF, OBJ([shape], [shape], [shape_t]), ERR), 1, [])],
	((GET_INDEX, (coord_get_index, NUMBER)),
	 (SET_INDEX, (coord_set_index, NUMBER))))
