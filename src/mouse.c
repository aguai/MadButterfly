#include "mb_types.h"
#include "mb_redraw_man.h"

#define ASSERT(x)

static void mouse_event_interpreter(event_t *evt, void *arg) {
    mouse_event_t *mevt = (mouse_event_t *)evt;
    redraw_man_t *rdman = (redraw_man_t *)arg;
    mb_obj_t *obj;
    mouse_event_t new_evt;
    coord_t *coord;
    shape_t *shape;
    
    ASSERT(evt->type == EVT_MOUSE_MOVE_RAW);
    
    obj = (mb_obj_t *)subject_get_object(evt->cur_tgt);
    if(rdman->last_mouse_over == obj) {
	evt->type = EVT_MOUSE_MOVE;
	return;
    }
    
    new_evt.x = mevt->x;
    new_evt.y = mevt->y;
    new_evt.but_state = mevt->but_state;
    new_evt.button = mevt->button;
    
    if(rdman->last_mouse_over != NULL) {
	new_evt.event.type = EVT_MOUSE_OUT;
	if(IS_MBO_COORD(rdman->last_mouse_over)) {
	    coord = (coord_t *)rdman->last_mouse_over;
	    subject_notify(coord->mouse_event, (event_t *)&new_evt);
	} else if(IS_MBO_SHAPES(rdman->last_mouse_over)) {
	    shape = (shape_t *)rdman->last_mouse_over;
	    ASSERT(shape->geo != NULL);
	    subject_notify(shape->geo->mouse_event, (event_t *)&new_evt); 
	}
    }

    new_evt.event.type = EVT_MOUSE_OVER;
    subject_notify(evt->cur_tgt, (event_t *)&new_evt);
    rdman->last_mouse_over = obj;
    
    evt->flags |= EVTF_STOP_NOTIFY;
}

/*! \brief This is event handler that observes addrm_monitor subject.
 *
 * addrm_monitor subject is a member of redraw manager objects.
 * Monitor of mouse event subjects of mb_obj_t objects are set to this
 * subject by redraw manager.
 *
 * addrm_monitor_hdlr() monitor adding and removing observers of mouse
 * event subjects, and install special observers to these subjects to handle
 * and interpret mouse events (EVT_MOUSE_MOVE_RAW).
 */
void addrm_monitor_hdlr(event_t *evt, void *arg) {
    monitor_event_t *mevt;
    redraw_man_t *rdman;
    mb_obj_t *obj;
    mb_prop_store_t *props;
    observer_t *observer;
    int cnt = 0;
    
    mevt = (monitor_event_t *)evt;
    rdman = (redraw_man_t *)evt->tgt;
    obj = (mb_obj_t *)subject_get_object(mevt->subject);
    props = mb_obj_prop_store(obj);

    switch(evt->type) {
    case EVT_MONITOR_ADD:
	if(!mb_prop_has(props, PROP_MEVT_OB_CNT))
	    cnt = 0;
	else
	    cnt = (int)mb_prop_get(props, PROP_MEVT_OB_CNT);
	
	cnt++;
	mb_prop_set(props, PROP_MEVT_OB_CNT, (void *)cnt);
	if(cnt == 1) {
	    observer =
		subject_add_event_observer_head(mevt->subject,
						EVT_MOUSE_MOVE_RAW,
						mouse_event_interpreter,
						rdman);
	    ASSERT(observer != NULL);
	    mb_prop_set(props, PROP_MEVT_OBSERVER, observer);
	}
	break;
	
    case EVT_MONITOR_REMOVE:
	cnt = (int)mb_prop_get(props, PROP_MEVT_OB_CNT);
	cnt--;
	mb_prop_set(props, PROP_MEVT_OB_CNT, (void *)cnt);
	if(cnt == 1) {
	    observer = (observer_t *)mb_prop_get(props, PROP_MEVT_OBSERVER);
	    subject_remove_observer(mevt->subject, observer);
	    mb_prop_del(props, PROP_MEVT_OBSERVER);
	}
	break;
	
    case EVT_MONITOR_FREE:
	break;
    }
}

