#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include "redraw_man.h"
#include "mb_timer.h"
#include "X_supp.h"


static unsigned int get_button_state(unsigned int state) {
    unsigned int but = 0;

    if(state & Button1Mask)
	but |= MOUSE_BUT1;
    if(state & Button2Mask)
	but |= MOUSE_BUT2;
    if(state & Button3Mask)
	but |= MOUSE_BUT3;
    
    return but;
}

static unsigned int get_button(unsigned int button) {
    switch(button) {
    case Button1:
	return MOUSE_BUT1;
    case Button2:
	return MOUSE_BUT2;
    case Button3:
	return MOUSE_BUT3;
    }
    return 0;
}

/*! \brief Notify observers of the shape at specified
 *	position for mouse event.
 *
 * Observers of parent shapes may be called if the subject is not
 * with SUBF_STOP_PROPAGATE flag.  The subject of mouse event
 * for a shape is returned by sh_get_mouse_event_subject().
 */
static void notify_shapes(redraw_man_t *rdman,
			  co_aix x, co_aix y, int etype,
			  unsigned int state,
			  unsigned int button) {
    mouse_event_t mouse_event;
    shape_t *shape;
    subject_t *subject;
    ob_factory_t *factory;
    int in_stroke;

    mouse_event.event.type = etype;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.but_state = state;
    mouse_event.button = button;
    
    shape = find_shape_at_pos(rdman, x, y,
			      &in_stroke);
    if(shape == NULL)
	return;
    subject = sh_get_mouse_event_subject(shape);
    factory = rdman_get_ob_factory(rdman);
    
    subject_notify(factory, subject, (event_t *)&mouse_event);
}

/*! \brief Dispatch all X events in the queue.
 */
static void handle_x_event(X_MB_runtime_t *rt) {
    Display *display = rt->display;
    redraw_man_t *rdman = rt->rdman;
    XEvent evt;
    XMotionEvent *mevt;
    XButtonEvent *bevt;
    XExposeEvent *eevt;
    co_aix x, y, w, h;

    int eflag = 0;
    int ex1=0, ey1=0, ex2=0, ey2=0;

    unsigned int state, button;
    int r;

    while(XEventsQueued(display, QueuedAfterReading) > 0) {
	r = XNextEvent(display, &evt);
	if(r == -1)
	    break;

	switch(evt.type) {
	case ButtonPress:
	    bevt = (XButtonEvent *)&evt;
	    x = bevt->x;
	    y = bevt->y;
	    state = get_button_state(bevt->state);
	    button = get_button(bevt->button);

	    notify_shapes(rdman, x, y, EVT_MOUSE_BUT_PRESS,
			  state, button);
	    break;

	case ButtonRelease:
	    bevt = (XButtonEvent *)&evt;
	    x = bevt->x;
	    y = bevt->y;
	    state = get_button_state(bevt->state);
	    button = get_button(bevt->button);

	    notify_shapes(rdman, x, y, EVT_MOUSE_BUT_RELEASE,
			  state, button);
	    break;

	case MotionNotify:
	    mevt = (XMotionEvent *)&evt;
	    x = mevt->x;
	    y = mevt->y;
	    state = get_button_state(mevt->state);

	    notify_shapes(rdman, x, y, EVT_MOUSE_MOVE, state, 0);
	    break;

	case Expose:
	    eevt = &evt.xexpose;
	    x = eevt->x;
	    y = eevt->y;
	    w = eevt->width;
	    h = eevt->height;

	    if(eflag) {
		if(x < ex1)
		    ex1 = x;
		if(y < ey1)
		    ey1 = y;
		if((x + w) > ex2)
		    ex2 = x + w;
		if((y + h) > ey2)
		    ey2 = y + h;
	    } else {
		ex1 = x;
		ey1 = y;
		ex2 = x + w;
		ey2 = y + h;
		eflag = 1;
	    }
	    break;
	}
    }
    if(eflag) {
	rdman_redraw_area(rdman, ex1, ey1, (ex2 - ex1), (ey2 - ey1));
	eflag = 0;
    }
    XFlush(display);
}

/*! \brief Handle connection coming data and timeout of timers.
 *
 * \param display is a Display returned by XOpenDisplay().
 * \param rdman is a redraw manager.
 * \param tman is a timer manager.
 *
 * The display is managed by specified rdman and tman.  rdman draws
 * on the display, and tman trigger actions according timers.
 */
void X_MB_handle_connection(X_MB_runtime_t *rt) {
    Display *display = rt->display;
    redraw_man_t *rdman = rt->rdman;
    mb_tman_t *tman = rt->tman;
    int fd;
    mb_timeval_t now, tmo;
    struct timeval tv;
    fd_set rfds;
    int nfds;
    int r;

    rdman_redraw_all(rdman);
    XFlush(display);

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
	    handle_x_event(rt);
	}
    }
}

#define ERR -1
#define OK 0

static int X_init_connection(const char *display_name,
			     int w, int h,
			     Display **displayp,
			     Visual **visualp,
			     Window *winp) {
    Display *display;
    Window root, win;
    Visual *visual;
    int screen;
    XSetWindowAttributes wattr;
    int depth;
    int x, y;
    int r;

    display = XOpenDisplay(display_name);
    if(display == NULL)
	return ERR;

    screen = DefaultScreen(display);
    root = DefaultRootWindow(display);
    visual = DefaultVisual(display, screen);
    depth = DefaultDepth(display, screen);
    wattr.override_redirect = False;
    x = 10;
    y = 10;
    win = XCreateWindow(display, root,
			 x, y,
			 w, h,
			 1, depth, InputOutput, visual,
			 CWOverrideRedirect, &wattr);
    r = XMapWindow(display, win);
    if(r == -1) {
	XCloseDisplay(display);
	return ERR;
    }

    XSelectInput(display, win, PointerMotionMask | ExposureMask);
    XFlush(display);

    *displayp = display;
    *visualp = visual;
    *winp = win;

    return OK;
}

/*! \brief Initialize a MadButterfy runtime for Xlib.
 *
 * It setups a runtime environment to run MadButterfly with Xlib.
 * Users should specify width and height of the opening window.
 */
int X_MB_init(const char *display_name,
	      int w, int h, X_MB_runtime_t *xmb_rt) {
    memset(xmb_rt, 0, sizeof(X_MB_runtime_t));

    xmb_rt->w = w;
    xmb_rt->h = h;
    X_init_connection(display_name, w, h, &xmb_rt->display,
		      &xmb_rt->visual, &xmb_rt->win);

    xmb_rt->surface =
	cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    
    xmb_rt->backend_surface =
	cairo_xlib_surface_create(xmb_rt->display,
				  xmb_rt->win,
				  xmb_rt->visual,
				  w, h);

    xmb_rt->cr = cairo_create(xmb_rt->surface);
    xmb_rt->backend_cr = cairo_create(xmb_rt->backend_surface);

    cairo_set_source_surface(xmb_rt->backend_cr, xmb_rt->surface, 0, 0);

    xmb_rt->rdman = (redraw_man_t *)malloc(sizeof(redraw_man_t));
    redraw_man_init(xmb_rt->rdman, xmb_rt->cr, xmb_rt->backend_cr);

    xmb_rt->tman = mb_tman_new();

    xmb_rt->last = NULL;

    return OK;
}

void X_MB_destroy(X_MB_runtime_t *xmb_rt) {
    if(xmb_rt->rdman) {
	redraw_man_destroy(xmb_rt->rdman);
	free(xmb_rt->rdman);
    }

    if(xmb_rt->tman)
	mb_tman_free(xmb_rt->tman);

    if(xmb_rt->cr)
	cairo_destroy(xmb_rt->cr);
    if(xmb_rt->backend_cr)
	cairo_destroy(xmb_rt->backend_cr);

    if(xmb_rt->surface)
	cairo_surface_destroy(xmb_rt->surface);
    if(xmb_rt->backend_surface)
	cairo_surface_destroy(xmb_rt->backend_surface);

    if(xmb_rt->display)
	XCloseDisplay(xmb_rt->display);
}

