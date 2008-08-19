#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "redraw_man.h"
#include "mb_timer.h"


/*! \brief Dispatch all events in the queue.
 */
static void handle_x_event(Display *display,
			   redraw_man_t *rdman,
			   mb_tman_t *tman) {
    XEvent evt;
    XMotionEvent *mevt;
    mouse_event_t mouse_event;
    shape_t *shape;
    subject_t *subject;
    ob_factory_t *factory;
    co_aix x, y;
    int in_stroke;
    int but;
    int r;

    while(XEventsQueued(display, QueuedAfterReading) > 0) {
	r = XNextEvent(display, &evt);
	if(r == -1)
	    break;

	switch(evt.type) {
	case MotionNotify:
	    mevt = (XMotionEvent *)&evt;
	    x = mevt->x;
	    y = mevt->y;
	    but = 0;
	    if(mevt->state & Button1Mask)
		but |= MOUSE_BUT1;
	    if(mevt->state & Button2Mask)
		but |= MOUSE_BUT2;
	    if(mevt->state & Button3Mask)
		but |= MOUSE_BUT3;

	    mouse_event.event.type = EVT_MOUSE_MOVE;
	    mouse_event.x = x;
	    mouse_event.y = y;
	    mouse_event.button = but;

	    shape = find_shape_at_pos(rdman, x, y,
				      &in_stroke);
	    subject = sh_get_mouse_event_subject(shape);
	    factory = rdman_get_ob_factory(rdman);

	    subject_notify(factory, subject, (event_t *)&mouse_event);
	    break;

	case Expose:
	    rdman_redraw_area(rdman, evt.xexpose.x, evt.xexpose.y,
			      evt.xexpose.width, evt.xexpose.height);
	    break;
	}
    }
    rdman_redraw_changed(rdman);
    XFlush(display);
}

/*! \brief Handle connection coming data and timeout of timers.
 */
void X_handle_connection(Display *display,
			 redraw_man_t *rdman,
			 mb_tman_t *tman) {
    int fd;
    mb_timeval_t now, tmo;
    struct timeval tv;
    fd_set rfds;
    int nfds;
    int r;

    fd = XConnectionNumber(display);
    nfds = fd + 1;
    while(1) {
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	
	get_now(&now);
	r = mb_tman_next_timeout(tman, &now, &tmo);

	if(r == 0) {
	    tv.tv_sec = MB_TIMEVAL_SEC(&tmo);
	    tv.tv_usec = MB_TIMEVAL_USEC(&tmo);
	    r = select(nfds, &rfds, NULL, NULL, &tv);
	} else
	    r = select(nfds, &rfds, NULL, NULL, NULL);

	if(r == -1) {
	    perror("select");
	    break;
	}

	if(r == 0) {
	    get_now(&now);
	    mb_tman_handle_timeout(tman, &now);
	    rdman_redraw_changed(rdman);
	    XFlush(display);
	} else if(FD_ISSET(fd, &rfds)){
	    handle_x_event(display, rdman, tman);
	}
    }
}
