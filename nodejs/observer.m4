define([PROJ_PREFIX], [xnjsmb_auto_])dnl
dnl
STRUCT([observer], [observer_t],
       [INT([type])], [])

STRUCT([subject], [subject_t], [],
       [METHOD([add_event_observer], [_subject_add_event_observer],
       	       (INT([type]), FUNC([handler])), 2,
	       [OBJ([observer], [observer_t])]),
        METHOD([remove_observer], [_subject_remove_observer],
	       (OBJ([observer], [observer], [observer_t])), 1, [])])

STRUCT([event], [event_t],
       [INT([type]),
        ACCESSOR([tgt], [xnjsmb_event_tgt_getter], [xnjsmb_event_tgt_setter]),
        ACCESSOR([cur_tgt], [xnjsmb_event_cur_tgt_getter],
	    [xnjsmb_event_cur_tgt_setter]),
	INT([flags])], [], (([STMOD], [xnjsmb_event_mod])))
