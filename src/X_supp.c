// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo-xlib.h>
#include "mb_graph_engine.h"
#include "mb_redraw_man.h"
#include "mb_timer.h"
#include "mb_X_supp.h"
#include "mb_backend.h"
#include "mb_backend_utils.h"
#include "config.h"

#ifdef XSHM
/* \sa http://www.xfree86.org/current/mit-shm.html */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
static void XSHM_update(X_supp_runtime_t *xmb_rt);
#endif

#define ERR -1
#define OK 0

#define ASSERT(x)

#define ONLY_MOUSE_MOVE_RAW 1

static mb_timer_factory_t *_timer_factory = &tman_timer_factory;

/*! \ingroup xkb
 * @{
 */
struct _X_kb_info {
    int keycode_min, keycode_max;
    int ksym_per_code;
    KeySym *syms;
    subject_t *kbevents;
    ob_factory_t *ob_factory;
};

/* @} */

struct _X_supp_runtime {
    Display *display;
    Window win;
    Visual *visual;
    mbe_surface_t *surface, *backend_surface;
    mbe_pattern_t *surface_ptn;
    mbe_t *cr, *backend_cr;
    redraw_man_t *rdman;
    mb_img_ldr_t *img_ldr;
    int w, h;

    X_kb_info_t kbinfo;
    mb_IO_man_t *io_man;
    mb_timer_man_t *timer_man;

#ifndef ONLY_MOUSE_MOVE_RAW
    /* States */
    shape_t *last;
#endif

#ifdef XSHM
    XImage *ximage;
    XShmSegmentInfo shminfo;
#endif

    /* For handle connection */
    int io_hdl;

    /*
     * Following variables are used by handle_single_x_event()
     */
    int last_evt_type;	       /* Type of last event */
    int eflag;
    int ex1, ey1, ex2, ey2;    /* Aggregate expose events */
    int mflag;
    int mx, my;		       /* Position of last motion event */
    int mbut_state;	       /* Button state of last motion event */
};

static void _x_supp_handle_x_event(X_supp_runtime_t *rt);

/*! \defgroup x_supp_io IO manager for X.
 * @{
 */
#define MAX_MONITORS 200

typedef struct {
    int type;
    int fd;
    mb_IO_cb_t cb;
    void *data;
}  monitor_t;

struct _X_supp_IO_man {
    mb_IO_man_t io_man;
    monitor_t monitors[MAX_MONITORS];
    int n_monitor;
};

static int _x_supp_io_man_reg(struct _mb_IO_man *io_man,
			      int fd, MB_IO_TYPE type,
			      mb_IO_cb_t cb, void *data);
static void _x_supp_io_man_unreg(struct _mb_IO_man *io_man, int io_hdl);
static mb_IO_man_t *_x_supp_io_man_new(void);
static void _x_supp_io_man_free(mb_IO_man_t *io_man);

static mb_IO_factory_t _X_supp_default_io_factory = {
    _x_supp_io_man_new,
    _x_supp_io_man_free
};
static mb_IO_factory_t *_io_factory = &_X_supp_default_io_factory;

static struct _X_supp_IO_man _default_io_man = {
    {_x_supp_io_man_reg, _x_supp_io_man_unreg},
    {},			/* monitors */
    0			/* n_monitor */
};

static mb_IO_man_t *
_x_supp_io_man_new(void) {
    return (mb_IO_man_t *)&_default_io_man;
}

static void
_x_supp_io_man_free(mb_IO_man_t *io_man) {
}

static int
_x_supp_io_man_reg(struct _mb_IO_man *io_man,
		   int fd, MB_IO_TYPE type, mb_IO_cb_t cb, void *data) {
    struct _X_supp_IO_man *xmb_io_man = (struct _X_supp_IO_man *)io_man;
    int i;

    for(i = 0; i < xmb_io_man->n_monitor; i++) {
        if (xmb_io_man->monitors[i].type == MB_IO_DUMMY)
	    break;
    }
    if (i == MAX_MONITORS)
	return ERR;
    
    xmb_io_man->monitors[i].type = type;
    xmb_io_man->monitors[i].fd = fd;
    xmb_io_man->monitors[i].cb = cb;
    xmb_io_man->monitors[i].data = data;
    i++;
    if(i > xmb_io_man->n_monitor)
	xmb_io_man->n_monitor = i;
    return i - 1;
}

static void
_x_supp_io_man_unreg(struct _mb_IO_man *io_man, int io_hdl) {
    struct _X_supp_IO_man *xmb_io_man = (struct _X_supp_IO_man *)io_man;
    
    ASSERT(io_hdl < xmb_io_man->n_monitor);
    xmb_io_man->monitors[io_hdl].type = MB_IO_DUMMY;
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
static void
_x_supp_event_loop(mb_rt_t *rt) {
    struct _X_supp_runtime *xmb_rt = (struct _X_supp_runtime *)rt;
    struct _X_supp_IO_man *io_man = (struct _X_supp_IO_man *)xmb_rt->io_man;
    mb_timer_man_t *timer_man = (mb_timer_man_t *)xmb_rt->timer_man;
    redraw_man_t *rdman;
    mb_tman_t *tman = tman_timer_man_get_tman(timer_man);
    mb_timeval_t now, tmo;
    struct timeval tv;
    fd_set rfds, wfds;
    int nfds = 0;
    int r, r1,i;

    rdman = mb_runtime_rdman(rt);
    
    _x_supp_handle_x_event(xmb_rt);

    while(1) {
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
        for(i = 0; i < io_man->n_monitor; i++) {
	    if(io_man->monitors[i].type == MB_IO_R ||
	       io_man->monitors[i].type == MB_IO_RW) {
		FD_SET(io_man->monitors[i].fd, &rfds);
		nfds = MB_MAX(nfds, io_man->monitors[i].fd + 1);
	    }
	    if(io_man->monitors[i].type == MB_IO_W ||
	       io_man->monitors[i].type == MB_IO_RW) {
		FD_SET(io_man->monitors[i].fd, &wfds);
		nfds = MB_MAX(nfds, io_man->monitors[i].fd + 1);
	    }
        }

	get_now(&now);
	r = mb_tman_next_timeout(tman, &now, &tmo);

	if(r == 0) {
	    tv.tv_sec = MB_TIMEVAL_SEC(&tmo);
	    tv.tv_usec = MB_TIMEVAL_USEC(&tmo);
	    r1 = select(nfds, &rfds, NULL, NULL, &tv);
	} else
	    r1 = select(nfds, &rfds, NULL, NULL, NULL);

	if(r1 == -1) {
	    perror("select");
	    break;
	}

	if(r1 == 0) {
	    get_now(&now);
	    mb_tman_handle_timeout(tman, &now);
	    rdman_redraw_changed(rdman);
#ifdef XSHM
	    XSHM_update(xmb_rt);
#endif
	    XFlush(xmb_rt->display);
	} else {
            for(i = 0; i < io_man->n_monitor; i++) {
	        if(io_man->monitors[i].type == MB_IO_R ||
		   io_man->monitors[i].type == MB_IO_RW) {
		    if(FD_ISSET(io_man->monitors[i].fd, &rfds))
		    	io_man->monitors[i].cb(i, io_man->monitors[i].fd,
					       MB_IO_R,
					       io_man->monitors[i].data);
		}
		if(io_man->monitors[i].type == MB_IO_W ||
		   io_man->monitors[i].type == MB_IO_RW) {
		    if(FD_ISSET(io_man->monitors[i].fd, &wfds))
			io_man->monitors[i].cb(i, io_man->monitors[i].fd,
					       MB_IO_W,
					       io_man->monitors[i].data);
		}
            }
	}
    }
}

/* @} */

#ifdef XSHM
static void
XSHM_update(X_supp_runtime_t *xmb_rt) {
    GC gc;

    gc = DefaultGC(xmb_rt->display, DefaultScreen(xmb_rt->display));
    if(xmb_rt->ximage) {	/* support XSHM */
	XShmPutImage(xmb_rt->display,
		     xmb_rt->win,
		     gc,
		     xmb_rt->ximage,
		     0, 0, 0, 0,
		     xmb_rt->w, xmb_rt->h, 0);
    }
}
#endif

/*! \defgroup xkb X Keyboard Handling
 *
 * Accept keyboard events from X server and delivery it to
 * application through observer pattern.  There is a subject,
 * per X-connection, for that.
 * @{
 */
static int keycode2sym(X_kb_info_t *kbinfo, unsigned int keycode) {
    int sym_idx;
    int sym;

    sym_idx = kbinfo->ksym_per_code * (keycode - kbinfo->keycode_min);
    sym =  kbinfo->syms[sym_idx];
    return sym;
}

static int X_kb_init(X_kb_info_t *kbinfo, Display *display,
		     redraw_man_t *rdman) {
    int n_syms;
    ob_factory_t *factory;
    int r;

    r = XDisplayKeycodes(display,
			 &kbinfo->keycode_min,
			 &kbinfo->keycode_max);
    if(r == 0)
	return ERR;

    n_syms = kbinfo->keycode_max - kbinfo->keycode_min + 1;
    kbinfo->syms = XGetKeyboardMapping(display, kbinfo->keycode_min,
				       n_syms,
				       &kbinfo->ksym_per_code);
    if(kbinfo->syms == NULL)
	return ERR;

    factory = rdman_get_ob_factory(rdman);
    kbinfo->kbevents = subject_new(factory, kbinfo, OBJT_KB);
    if(kbinfo->kbevents == NULL)
	return ERR;
    /*! \todo Make sure ob_factory is still need. */
    kbinfo->ob_factory = factory;

    return OK;
}

static void X_kb_destroy(X_kb_info_t *kbinfo) {
    subject_free(kbinfo->kbevents);
    XFree(kbinfo->syms);
}

/*! \brief Accept X keyboard events from _x_supp_handle_x_event() and
 *         dispatch it.
 */
static void X_kb_handle_event(X_kb_info_t *kbinfo, XKeyEvent *xkey) {
    unsigned int code;
    int sym;
    X_kb_event_t event;

    code = xkey->keycode;
    sym = keycode2sym(kbinfo, code);
    if(xkey->type == KeyPress)
	event.event.type = EVT_KB_PRESS;
    else if(xkey->type == KeyRelease)
	event.event.type = EVT_KB_RELEASE;
    event.event.tgt = event.event.cur_tgt = kbinfo->kbevents;
    event.keycode = code;
    event.sym = sym;

    subject_notify(kbinfo->kbevents, &event.event);
}
/* @} */

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
static void notify_coord_or_shape(redraw_man_t *rdman,
				   mb_obj_t *obj,
				   co_aix x, co_aix y, int etype,
				   unsigned int state,
				   unsigned int button) {
    mouse_event_t mouse_event;
    subject_t *subject;

    mouse_event.event.type = etype;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.but_state = state;
    mouse_event.button = button;

    if(IS_MBO_SHAPES(obj))
	subject = sh_get_mouse_event_subject((shape_t *)obj);
    else
	subject = coord_get_mouse_event((coord_t *)obj);

    subject_notify(subject, (event_t *)&mouse_event);
}

/*! \brief Handle motion event.
 */
static void
handle_motion_event(X_supp_runtime_t *rt) {
    redraw_man_t *rdman = rt->rdman;
    int x, y;
    int state;
    shape_t *shape;
    coord_t *root;
    int in_stroke;
    
    x = rt->mx;
    y = rt->my;
    state = rt->mbut_state;
    
    shape = find_shape_at_pos(rdman, x, y,
			      &in_stroke);
#ifdef ONLY_MOUSE_MOVE_RAW
    if(shape != NULL) {
	notify_coord_or_shape(rdman, (mb_obj_t *)shape,
			      x, y, EVT_MOUSE_MOVE_RAW, state, 0);
    } else {
	root = rdman_get_root(rdman);
	notify_coord_or_shape(rdman, (mb_obj_t *)root,
			      x, y, EVT_MOUSE_MOVE_RAW, state, 0);
    }
#else
    if(shape != NULL) {
	if(rt->last != shape) {
	    if(rt->last)
		notify_coord_or_shape(rdman, rt->last, x, y,
				      EVT_MOUSE_OUT, state, 0);
	    notify_coord_or_shape(rdman, shape, x, y,
				  EVT_MOUSE_OVER, state, 0);
	    rt->last = shape;
	} else
	    notify_coord_or_shape(rdman, shape, x, y,
				  EVT_MOUSE_MOVE, state, 0);
    } else {
	if(rt->last) {
	    notify_coord_or_shape(rdman, rt->last, x, y,
				  EVT_MOUSE_OUT, state, 0);
	    rt->last = NULL;
	}
    }
#endif
    
    rt->mflag = 0;
}

/*! \brief Redraw exposed area.
 */
static void
handle_expose_event(X_supp_runtime_t *rt) {
    redraw_man_t *rdman = rt->rdman;
    int ex1, ey1, ex2, ey2;

    ex1 = rt->ex1;
    ey1 = rt->ey1;
    ex2 = rt->ex2;
    ey2 = rt->ey2;
    
    rdman_redraw_area(rdman, ex1, ey1, (ex2 - ex1), (ey2 - ey1));
    
    rt->eflag = 0;
}

/*! \brief Handle single X event and maintain internal states.
 *
 * It keeps internal state in rt to improve performance.
 */
static void
handle_single_x_event(X_supp_runtime_t *rt, XEvent *evt) {
    redraw_man_t *rdman = rt->rdman;
    XMotionEvent *mevt;
    XButtonEvent *bevt;
    XExposeEvent *eevt;
    XKeyEvent *xkey;
    int x, y, w, h;

    shape_t *shape;

    unsigned int state, button;
    int in_stroke;

    if(evt->type != MotionNotify && rt->mflag)
	handle_motion_event(rt);

    switch(evt->type) {
    case ButtonPress:
	bevt = (XButtonEvent *)evt;
	x = bevt->x;
	y = bevt->y;
	state = get_button_state(bevt->state);
	button = get_button(bevt->button);

	shape = find_shape_at_pos(rdman, x, y,
				  &in_stroke);
	if(shape)
	    notify_coord_or_shape(rdman, (mb_obj_t *)shape,
				  x, y, EVT_MOUSE_BUT_PRESS,
				  state, button);
	break;

    case ButtonRelease:
	bevt = (XButtonEvent *)evt;
	x = bevt->x;
	y = bevt->y;
	state = get_button_state(bevt->state);
	button = get_button(bevt->button);

	shape = find_shape_at_pos(rdman, x, y,
				  &in_stroke);
	if(shape)
	    notify_coord_or_shape(rdman, (mb_obj_t *)shape,
				  x, y, EVT_MOUSE_BUT_RELEASE,
				  state, button);
	break;

    case MotionNotify:
	mevt = (XMotionEvent *)evt;
	rt->mx = mevt->x;
	rt->my = mevt->y;
	rt->mbut_state = get_button_state(mevt->state);
	rt->mflag = 1;
	break;

    case KeyPress:
    case KeyRelease:
	xkey = &evt->xkey;
	X_kb_handle_event(&rt->kbinfo, xkey);
	break;

    case Expose:
	eevt = &evt->xexpose;
	x = eevt->x;
	y = eevt->y;
	w = eevt->width;
	h = eevt->height;

	if(rt->eflag) {
	    if(x < rt->ex1)
		rt->ex1 = x;
	    if(y < rt->ey1)
		rt->ey1 = y;
	    if((x + w) > rt->ex2)
		rt->ex2 = x + w;
	    if((y + h) > rt->ey2)
		rt->ey2 = y + h;
	} else {
	    rt->ex1 = x;
	    rt->ey1 = y;
	    rt->ex2 = x + w;
	    rt->ey2 = y + h;
	    rt->eflag = 1;
	}
	break;
    }
}

/*! \brief Call when no more event in an event iteration.
 *
 * No more event means event queue is emplty.  This function will
 * perform some actions according current internal state.
 */
static void
no_more_event(X_supp_runtime_t *rt) {
    if(rt->mflag)
	handle_motion_event(rt);
    if(rt->eflag)
	handle_expose_event(rt);
}

/*! \brief Dispatch all X events in the queue.
 */
static void _x_supp_handle_x_event(X_supp_runtime_t *rt) {
    Display *display = rt->display;
    XEvent evt;
    int r;

    /* XXX: For some unknown reason, it causes a segmentation fault to
     *      called XEventsQueued() after receiving first Expose event
     *      and before redraw for the event.
     */
    while(XEventsQueued(display, QueuedAfterReading) > 0) {
	r = XNextEvent(display, &evt);
	if(r == -1)
	    break;

	handle_single_x_event(rt, &evt);
    }
    no_more_event(rt);
    
#ifdef XSHM
    XSHM_update(rt);
#endif
    XFlush(display);
}

static void
_x_supp_handle_connection(int hdl, int fd, MB_IO_TYPE type, void *data) {
    X_supp_runtime_t *xmb_rt = (X_supp_runtime_t *)data;

    _x_supp_handle_x_event(xmb_rt);
}

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
    int draw_root = 0;
    const char *disp_name;
    char disp_buf[32];
    int cp;
    int r;

    /*
     * Support drawing on the root window.
     */
    disp_name = display_name;
    if(strstr(display_name, ":root") != NULL) {
	draw_root = 1;
	cp = strlen(display_name) - 5;
	if(cp >= 32)
	    cp = 31;
	memcpy(disp_buf, display_name, cp);
	disp_buf[cp] = 0;
	disp_name = disp_buf;
    }

    display = XOpenDisplay(disp_name);
    if(display == NULL)
	return ERR;

    screen = DefaultScreen(display);
    root = DefaultRootWindow(display);
    visual = DefaultVisual(display, screen);
    depth = DefaultDepth(display, screen);
    wattr.override_redirect = False;
    x = 10;
    y = 10;
    if(draw_root)
	win = RootWindowOfScreen(ScreenOfDisplay(display, screen));
    else {
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
    }

    XSelectInput(display, win, PointerMotionMask | ExposureMask |
		 ButtonPressMask | ButtonReleaseMask |
		 KeyPressMask | KeyReleaseMask);
    XFlush(display);

    *displayp = display;
    *visualp = visual;
    *winp = win;

    return OK;
}

#ifdef XSHM
static void
xshm_destroy(X_supp_runtime_t *xmb_rt) {
    XShmSegmentInfo *shminfo;

    shminfo = &xmb_rt->shminfo;

    if(xmb_rt->shminfo.shmaddr) {
	XShmDetach(xmb_rt->display, shminfo);
    }

    if(xmb_rt->ximage) {
	XDestroyImage(xmb_rt->ximage);
	xmb_rt->ximage = NULL;
    }

    if(shminfo->shmaddr) {
	shmdt(shminfo->shmaddr);
	shminfo->shmaddr = NULL;
    }

    if(shminfo->shmid) {
	shmctl(shminfo->shmid, IPC_RMID, 0);
	shminfo->shmid = 0;
    }
}

static void
xshm_init(X_supp_runtime_t *xmb_rt) {
    Display *display;
    Visual *visual;
    XImage *ximage;
    int screen;
    int depth;
    int support_shm;
    int mem_sz;
    XShmSegmentInfo *shminfo;
    int surf_fmt;

    display = xmb_rt->display;
    visual = xmb_rt->visual;
    shminfo = &xmb_rt->shminfo;

    support_shm = XShmQueryExtension(display);
    if(!support_shm)
	return;

    screen = DefaultScreen(display);
    depth = DefaultDepth(display, screen);

    if(depth != 24 && depth != 32)
	return;

    xmb_rt->ximage = XShmCreateImage(display, visual, depth,
				     ZPixmap, NULL, shminfo,
				     xmb_rt->w, xmb_rt->h);
    ximage = xmb_rt->ximage;

    mem_sz = ximage->bytes_per_line * ximage->height;
    shminfo->shmid = shmget(IPC_PRIVATE, mem_sz, IPC_CREAT | 0777);
    if(shminfo->shmid == -1) {
	xshm_destroy(xmb_rt);
	return;
    }

    shminfo->shmaddr = shmat(shminfo->shmid, 0, 0);
    ximage->data = shminfo->shmaddr;

    shminfo->readOnly = 0;

    XShmAttach(display, shminfo);

    switch(depth) {
    case 24: surf_fmt = CAIRO_FORMAT_RGB24; break;
    case 32: surf_fmt = CAIRO_FORMAT_ARGB32; break;
    }

    xmb_rt->backend_surface =
	mbe_image_surface_create_for_data((unsigned char *)ximage->data,
					  surf_fmt,
					  xmb_rt->w,
					  xmb_rt->h,
					  ximage->bytes_per_line);
    if(xmb_rt->backend_surface == NULL)
	xshm_destroy(xmb_rt);
}
#endif /* XSHM */

/*! \brief Initialize a MadButterfy runtime for Xlib.
 *
 * This one is very like _x_supp_init(), except it accepts a
 * X_supp_runtime_t object initialized with a display connected to a X
 * server and an opened window.
 *
 * Following field of the X_supp_runtime_t object should be initialized.
 *   - w, h
 *   - win
 *   - display
 *   - visual
 */
static int
_x_supp_init_with_win_internal(X_supp_runtime_t *xmb_rt) {
    mb_img_ldr_t *img_ldr;
    int w, h;
    int disp_fd;

    w = xmb_rt->w;
    h = xmb_rt->h;

#ifdef XSHM
    xshm_init(xmb_rt);
#endif

    xmb_rt->surface =
	mbe_image_surface_create(MB_IFMT_ARGB32, w, h);

    xmb_rt->surface_ptn =
	mbe_pattern_create_for_surface(xmb_rt->surface);

    if(xmb_rt->backend_surface == NULL) /* xshm_init() may create one */
	xmb_rt->backend_surface =
	    mbe_xlib_surface_create(xmb_rt->display,
				    xmb_rt->win,
				    xmb_rt->visual,
				    w, h);

    xmb_rt->cr = mbe_create(xmb_rt->surface);
    xmb_rt->backend_cr = mbe_create(xmb_rt->backend_surface);

    mbe_set_source(xmb_rt->backend_cr, xmb_rt->surface_ptn);

    xmb_rt->rdman = (redraw_man_t *)malloc(sizeof(redraw_man_t));
    redraw_man_init(xmb_rt->rdman, xmb_rt->cr, xmb_rt->backend_cr);
    // FIXME: This is a wired loopback reference. This is inly required when we need
    //        to get the xmb_rt->tman for the animation. We should relocate the tman
    //	      to the redraw_man_t instead.
    xmb_rt->rdman->rt = xmb_rt;

    xmb_rt->io_man = mb_io_man_new(_io_factory);
    xmb_rt->timer_man = mb_timer_man_new(_timer_factory);

    img_ldr = simple_mb_img_ldr_new("");
    xmb_rt->img_ldr = img_ldr;
    /*! \todo Remove rdman_set_img_ldr() */
    rdman_set_img_ldr(xmb_rt->rdman, img_ldr); /* this is ncessary? */

#ifndef ONLY_MOUSE_MOVE_RAW
    xmb_rt->last = NULL;
#endif

    X_kb_init(&xmb_rt->kbinfo, xmb_rt->display, xmb_rt->rdman);

    disp_fd = XConnectionNumber(xmb_rt->display);
    xmb_rt->io_hdl = mb_io_man_reg(xmb_rt->io_man, disp_fd,
				   MB_IO_R,
				   _x_supp_handle_connection,
				   xmb_rt);

    return OK;
}

/*! \brief Initialize a MadButterfy runtime for Xlib.
 *
 * It setups a runtime environment to run MadButterfly with Xlib.
 * Users should specify width and height of the opening window.
 */
static int _x_supp_init(X_supp_runtime_t *xmb_rt, const char *display_name,
			int w, int h) {
    int r;

    memset(xmb_rt, 0, sizeof(X_supp_runtime_t));

    xmb_rt->w = w;
    xmb_rt->h = h;
    r = X_init_connection(display_name, w, h, &xmb_rt->display,
			  &xmb_rt->visual, &xmb_rt->win);
    if(r != OK)
	return ERR;

    r = _x_supp_init_with_win_internal(xmb_rt);

    return r;
}

/*! \brief Initialize a MadButterfly runtime for a window of X.
 *
 * Runtimes initialized with this function should be destroyed with
 * x_supp_destroy_keep_win().
 */
static int
_x_supp_init_with_win(X_supp_runtime_t *xmb_rt,
		      Display *display, Window win) {
    XWindowAttributes attrs;
    int r;

    r = XGetWindowAttributes(display, win, &attrs);
    if(r == 0)
	return ERR;
    
    memset(xmb_rt, 0, sizeof(X_supp_runtime_t));

    xmb_rt->display = display;
    xmb_rt->win = win;
    xmb_rt->visual = attrs.visual;
    xmb_rt->w = attrs.width;
    xmb_rt->h = attrs.height;

    r = _x_supp_init_with_win_internal(xmb_rt);

    return r;
}

static void x_supp_destroy(X_supp_runtime_t *xmb_rt) {
    if(xmb_rt->rdman) {
	redraw_man_destroy(xmb_rt->rdman);
	free(xmb_rt->rdman);
    }

    if(xmb_rt->io_hdl)
	mb_io_man_unreg(xmb_rt->io_man, xmb_rt->io_hdl);

    if(xmb_rt->io_man)
	mb_io_man_free(_io_factory, xmb_rt->io_man);
    if(xmb_rt->timer_man)
	mb_timer_man_free(_timer_factory, xmb_rt->timer_man);

    if(xmb_rt->img_ldr)
	MB_IMG_LDR_FREE(xmb_rt->img_ldr);

    if(xmb_rt->cr)
	mbe_destroy(xmb_rt->cr);
    if(xmb_rt->backend_cr)
	mbe_destroy(xmb_rt->backend_cr);

    if(xmb_rt->surface)
	mbe_surface_destroy(xmb_rt->surface);
    if(xmb_rt->surface_ptn)
	mbe_pattern_destroy(xmb_rt->surface_ptn);
    if(xmb_rt->backend_surface)
	mbe_surface_destroy(xmb_rt->backend_surface);

    if(xmb_rt->display)
	XCloseDisplay(xmb_rt->display);

    X_kb_destroy(&xmb_rt->kbinfo);
}

/*! \brief Destroy a MadButterfly runtime initialized with
 *	_x_supp_init_with_win().
 *
 * Destroying a runtime with this function prevent the window and
 * display associated with the runtime being closed.
 */
static void
x_supp_destroy_keep_win(X_supp_runtime_t *xmb_rt) {
    Display *display;
    Window win;

    display = xmb_rt->display;
    xmb_rt->display = NULL;
    win = xmb_rt->win;
    xmb_rt->win = 0;

    x_supp_destroy(xmb_rt);
    
    xmb_rt->display = display;
    xmb_rt->win = win;
}

static mb_rt_t *
_x_supp_new(const char *display_name, int w, int h) {
    X_supp_runtime_t *rt;
    int r;

    rt = O_ALLOC(X_supp_runtime_t);
    if(rt == NULL)
	return NULL;

    r = _x_supp_init(rt, display_name, w, h);
    if(r != OK) {
	free(rt);
	return NULL;
    }

    return (mb_rt_t *)rt;
}

/*! \brief Create a new runtime for existed window for X.
 *
 * The object returned by this function must be free with
 * _x_supp_free_keep_win() to prevent the window from closed.
 */
static mb_rt_t *
_x_supp_new_with_win(MB_DISPLAY display, MB_WINDOW win) {
    X_supp_runtime_t *rt;
    int r;

    rt = O_ALLOC(X_supp_runtime_t);
    if(rt == NULL)
	return NULL;

    r = _x_supp_init_with_win(rt, display, win);
    if(r != OK) {
	free(rt);
	return NULL;
    }

    return (mb_rt_t *)rt;
}

static void
_x_supp_free(mb_rt_t *rt) {
    x_supp_destroy((X_supp_runtime_t *) rt);
    free(rt);
}

/*! \brief Free runtime created with _x_supp_new_with_win().
 */
static void
_x_supp_free_keep_win(mb_rt_t *rt) {
    x_supp_destroy_keep_win((X_supp_runtime_t *) rt);
    free(rt);
}

static subject_t *
_x_supp_kbevents(mb_rt_t *rt) {
    X_supp_runtime_t *xmb_rt = (X_supp_runtime_t *) rt;
    return xmb_rt->kbinfo.kbevents;
}

static redraw_man_t *
_x_supp_rdman(mb_rt_t *rt) {
    X_supp_runtime_t *xmb_rt = (X_supp_runtime_t *) rt;
    return xmb_rt->rdman;
}

static mb_timer_man_t *
_x_supp_timer_man(mb_rt_t *rt) {
    X_supp_runtime_t *xmb_rt = (X_supp_runtime_t *) rt;
    return xmb_rt->timer_man;
}

static ob_factory_t *
_x_supp_ob_factory(mb_rt_t *rt) {
    X_supp_runtime_t *xmb_rt = (X_supp_runtime_t *) rt;
    ob_factory_t *factory;

    factory = rdman_get_ob_factory(xmb_rt->rdman);
    return factory;
}

static mb_img_ldr_t *
_x_supp_img_ldr(mb_rt_t *rt) {
    X_supp_runtime_t *xmb_rt = (X_supp_runtime_t *) rt;
    mb_img_ldr_t *img_ldr;

    img_ldr = xmb_rt->img_ldr;

    return img_ldr;
}

static int
_x_supp_add_event(mb_rt_t *rt, int fd, MB_IO_TYPE type,
	       mb_IO_cb_t cb, void *data)
{
    X_supp_runtime_t *xmb_rt = (X_supp_runtime_t *) rt;
    mb_IO_man_t *io_man = xmb_rt->io_man;
    int hdl;

    hdl = mb_io_man_reg(io_man, fd, type, cb, data);
    return hdl;
}

static void
_x_supp_remove_event(mb_rt_t *rt, int hdl)
{
    X_supp_runtime_t *xmb_rt = (X_supp_runtime_t *) rt;
    mb_IO_man_t *io_man = xmb_rt->io_man;

    mb_io_man_unreg(io_man, hdl);
}

static int
_x_supp_flush(mb_rt_t *rt) {
    X_supp_runtime_t *xmb_rt = (X_supp_runtime_t *) rt;
    int r;

#ifdef XSHM
    XSHM_update(xmb_rt);
#endif
    r = XFlush(xmb_rt->display);
    return r == 0? ERR: OK;
}

static void
_x_supp_reg_IO_factory(mb_IO_factory_t *io_factory) {
    _io_factory = io_factory;
}

static void
_x_supp_reg_timer_factory(mb_timer_factory_t *timer_factory) {
    _timer_factory = timer_factory;
}

mb_backend_t mb_dfl_backend = { _x_supp_new,
				_x_supp_new_with_win,
				
				_x_supp_free,
				_x_supp_free_keep_win,
				_x_supp_add_event,
				_x_supp_remove_event,
				_x_supp_event_loop,
				_x_supp_flush,
				
				_x_supp_kbevents,
				_x_supp_rdman,
				_x_supp_timer_man,
				_x_supp_ob_factory,
				_x_supp_img_ldr,

				_x_supp_reg_IO_factory,
				_x_supp_reg_timer_factory,
};
