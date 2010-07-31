define([PROJ_PREFIX], [xnjsmb_auto_])

STRUCT([mb_obj], [mb_obj_t], [INT([obj_type])], [])

STRUCT([observer], [observer_t],
       [INT([type])], [])

STRUCT([subject], [subject_t], [],
       [METHOD([add_event_observer], [_subject_add_event_observer],
       	       (INT([type]), FUNC([handler])), 2,
	       [OBJ([observer], [observer_t])]),
        METHOD([remove_observer], [_subject_remove_observer],
	       (OBJ([observer], [observer_t])), 1, []),
        METHOD([get_object], [subject_get_object], (), 0,
	       [OBJ([mb_obj], [mb_obj_t])])])

STRUCT([event], [event_t],
       [INT([type]), OBJ([tgt], [subject], [subject_t]),
        OBJ([cur_tgt], [subject], [subject_t]), INT([flags])], [])
