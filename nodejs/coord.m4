define([PROJ_PREFIX], [xnjsmb_auto_])dnl
STRUCT([coord], [coord_t], 
        [
	ACCESSOR([opacity], [xnjsmb_coord_get_opacity],[xnjsmb_coord_set_opacity]),
	ACCESSOR([x], [xnjsmb_coord_get_x],[xnjsmb_coord_set_x]),
	ACCESSOR([y], [xnjsmb_coord_get_y],[xnjsmb_coord_set_y]),
	],
	[METHOD([add_shape], [xnjsmb_coord_add_shape],
		(SELF, OBJ([shape], [shape], [shape_t]), ERR), 1, []),
	 METHOD([remove], [xnjsmb_coord_remove], (SELF), 0, []),
	 METHOD([clone_from_subtree], [xnjsmb_coord_clone_from_subtree],
	 	(SELF, OBJ([src], [coord], [coord_t]), ERR), 1,
		[OBJ([coord], [coord_t])],
		(([MOD], [_xnjsmb_coord_clone_from_subtree_mod]))),
	 METHOD([show], [xnjsmb_coord_show], (SELF), 0, []),
	 METHOD([hide], [xnjsmb_coord_hide], (SELF), 0, []),
	 METHOD([get_child], [xnjsmb_coord_get_child],
	 	(SELF, INT(idx), ERR), 1, [VAL])],
	((GET_INDEX, (coord_get_index, NUMBER)),
	 (SET_INDEX, (coord_set_index, NUMBER)),
	 ([STMOD], [xnjsmb_coord_mod])))
