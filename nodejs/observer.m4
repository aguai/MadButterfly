STRUCT([event], [event_t],
       [INT([event]), OBJ([tgt], [subject]),
        OBJ([cur_tgt], [subject]), INT([flags])], [])

STRUCT([observer], [observer_t],
       [INT([type])], [])

STRUCT([subject], [subject_t], [],
       [METHOD([add_event_observer], [subject_add_event_observer],
       	       (INT([type]), FUNC([handler])), 2,
	       [OBJ([observer], [observer_t])]),
        METHOD([remove_observer], [subject_remove_observer],
	       (OBJ([observer], [observer_t])), 1, [])])

FUNCTION([load], [my_load], [INT(sz), STR(name)], 2, [OBJ([test],[test_t])])
