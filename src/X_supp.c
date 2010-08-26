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
#include "config.h"

#ifdef XSHM
/* \sa http://www.xfree86.org/current/mit-shm.html */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#define ERR -1
#define OK 0

#define ONLY_MOUSE_MOVE_RAW 1

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
#define MAX_MONITORS 200
typedef struct {
    int type;
    int fd;
    mb_eventcb_t f;
    void *arg;
}  monitor_t;
struct _X_MB_runtime {
    Display *display;
    Window win;
    Visual *visual;
    mbe_surface_t *surface, *backend_surface;
    mbe_pattern_t *surface_ptn;
    mbe_t *cr, *backend_cr;
    redraw_man_t *rdman;
    mb_tman_t *tman;
    mb_img_ldr_t *img_ldr;
    int w, h;

    X_kb_info_t kbinfo;
    monitor_t monitors[MAX_MONITORS];
    int n_monitor;

#ifndef ONLY_MOUSE_MOVE_RAW
    /* States */
    shape_t *last;
#endif

#ifdef XSHM
    XImage *ximage;
    XShmSegmentInfo shminfo;
#endif
};

#ifdef XSHM
static void
XSHM_update(X_MB_runtime_t *xmb_rt) {
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

/*! \brief Accept X keyboard events from handle_x_event() and dispatch it.
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

/*! \brief Dispatch all X events in the queue.
 */
static void handle_x_event(X_MB_runtime_t *rt) {
    Display *display = rt->display;
    redraw_man_t *rdman = rt->rdman;
    XEvent evt, peek_evt;
    XMotionEvent *mevt;
    XButtonEvent *bevt;
    XExposeEvent *eevt;
    XKeyEvent *xkey;
    co_aix x, y, w, h;

    int eflag = 0;
    int ex1=0, ey1=0, ex2=0, ey2=0;

    shape_t *shape;
    coord_t *root;

    unsigned int state, button;
    int in_stroke;
    int r;

    /* XXX: For some unknown reason, it causes a segmentation fault to
     *      called XEventsQueued() after receiving first Expose event
     *      and before redraw for the event.
     */
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

	    shape = find_shape_at_pos(rdman, x, y,
				      &in_stroke);
	    if(shape)
		notify_coord_or_shape(rdman, (mb_obj_t *)shape,
				      x, y, EVT_MOUSE_BUT_PRESS,
				      state, button);
	    break;

	case ButtonRelease:
	    bevt = (XButtonEvent *)&evt;
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
	    while(XEventsQueued(display, QueuedAfterReading) > 0) {
		r = XPeekEvent(display, &peek_evt);
		if(r == -1)
		    break;
		if(peek_evt.type != MotionNotify)
		    break;
		XNextEvent(display, &evt);
	    }
	    if(r == -1)
		break;
	    
	    mevt = (XMotionEvent *)&evt;
	    x = mevt->x;
	    y = mevt->y;
	    state = get_button_state(mevt->state);

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
	    break;

	case KeyPress:
	case KeyRelease:
	    xkey = &evt.xkey;
	    X_kb_handle_event(&rt->kbinfo, xkey);
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
#ifdef XSHM
    XSHM_update(rt);
#endif
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
void X_MB_handle_connection(void *be) {
    X_MB_runtime_t *rt = (X_MB_runtime_t *) be;
    Display *display = rt->display;
    redraw_man_t *rdman = rt->rdman;
    mb_tman_t *tman = rt->tman;
    int fd;
    mb_timeval_t now, tmo;
    struct timeval tv;
    fd_set rfds,wfds;
    int nfds;
    int r, r1,i;

    handle_x_event(rt);

    fd = XConnectionNumber(display);
    nfds = fd + 1;
    while(1) {
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_SET(fd, &rfds);
        for(i=0;i<rt->n_monitor;i++) {
	    if (rt->monitors[i].type == MONITOR_READ)
		FD_SET(rt->monitors[i].fd, &rfds);
	    else if (rt->monitors[i].type == MONITOR_WRITE)
		FD_SET(rt->monitors[i].fd, &wfds);
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
	    XSHM_update(rt);
#endif
	    XFlush(display);
	} else if(FD_ISSET(fd, &rfds)){
	    handle_x_event(rt);
	} else {
            for(i=0;i<rt->n_monitor;i++) {
	        if (rt->monitors[i].type == MONITOR_READ) {
		    if (FD_ISSET(rt->monitors[i].fd, &rfds))
		    	rt->monitors[i].f(rt->monitors[i].fd,rt->monitors[i].arg);
		} else if (rt->monitors[i].type == MONITOR_WRITE) {
		    if (FD_ISSET(rt->monitors[i].fd, &wfds))
			rt->monitors[i].f(rt->monitors[i].fd,rt->monitors[i].arg);
		}
            }
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
xshm_destroy(X_MB_runtime_t *xmb_rt) {
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
xshm_init(X_MB_runtime_t *xmb_rt) {
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
 * This one is very like X_MB_init(), except it accepts a
 * X_MB_runtime_t object initialized with a display connected to a X
 * server and an opened window.
 *
 * Following field of the X_MB_runtime_t object should be initialized.
 *   - w, h
 *   - win
 *   - display
 *   - visual
 */
static int
X_MB_init_with_win(X_MB_runtime_t *xmb_rt) {
    mb_img_ldr_t *img_ldr;
    int w, h;
    
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

    xmb_rt->tman = mb_tman_new();

    img_ldr = simple_mb_img_ldr_new("");
    xmb_rt->img_ldr = img_ldr;
    rdman_set_img_ldr(xmb_rt->rdman, img_ldr);
    memset(xmb_rt->monitors,0,sizeof(xmb_rt->monitors));
    
#ifndef ONLY_MOUSE_MOVE_RAW
    xmb_rt->last = NULL;
#endif

    X_kb_init(&xmb_rt->kbinfo, xmb_rt->display, xmb_rt->rdman);

    return OK;
}

/*! \brief Initialize a MadButterfy runtime for Xlib.
 *
 * It setups a runtime environment to run MadButterfly with Xlib.
 * Users should specify width and height of the opening window.
 */
static int X_MB_init(const char *display_name,
	      int w, int h, X_MB_runtime_t *xmb_rt) {
    int r;
    
    memset(xmb_rt, 0, sizeof(X_MB_runtime_t));

    xmb_rt->w = w;
    xmb_rt->h = h;
    r = X_init_connection(display_name, w, h, &xmb_rt->display,
			  &xmb_rt->visual, &xmb_rt->win);
    if(r != OK)
	return ERR;

    r = X_MB_init_with_win(xmb_rt);
    
    return r;
}

static void X_MB_destroy(X_MB_runtime_t *xmb_rt) {
    if(xmb_rt->rdman) {
	redraw_man_destroy(xmb_rt->rdman);
	free(xmb_rt->rdman);
    }

    if(xmb_rt->tman)
	mb_tman_free(xmb_rt->tman);

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

void *X_MB_new(const char *display_name, int w, int h) {
    X_MB_runtime_t *rt;
    int r;

    rt = O_ALLOC(X_MB_runtime_t);
    if(rt == NULL)
	return NULL;

    r = X_MB_init(display_name, w, h, rt);
    if(r != OK) {
	free(rt);
	return NULL;
    }

    return rt;
}

void X_MB_free(void *rt) {
    X_MB_destroy((X_MB_runtime_t *) rt);
    free(rt);
}

subject_t *X_MB_kbevents(void *rt) {
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *) rt;
    return xmb_rt->kbinfo.kbevents;
}

redraw_man_t *X_MB_rdman(void *rt) {
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *) rt;
    return xmb_rt->rdman;
}

mb_tman_t *X_MB_tman(void *rt) {
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *) rt;
    return xmb_rt->tman;
}

ob_factory_t *X_MB_ob_factory(void *rt) {
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *) rt;
    ob_factory_t *factory;

    factory = rdman_get_ob_factory(xmb_rt->rdman);
    return factory;
}

mb_img_ldr_t *X_MB_img_ldr(void *rt) {
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *) rt;
    mb_img_ldr_t *img_ldr;

    img_ldr = xmb_rt->img_ldr;

    return img_ldr;
}

void X_MB_add_event(void *rt, int type, int fd, mb_eventcb_t f,void *arg)
{
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *) rt;
    int i;

    for(i=0;i<xmb_rt->n_monitor;i++) {
        if (xmb_rt->monitors[i].type == type && xmb_rt->monitors[i].fd == fd) {
	    xmb_rt->monitors[i].f = f;
	    xmb_rt->monitors[i].arg = arg;
	    return;
	}
    }
    for(i=0;i<xmb_rt->n_monitor;i++) {
        if (xmb_rt->monitors[i].type == 0) {
	    xmb_rt->monitors[i].type = type;
	    xmb_rt->monitors[i].fd = fd;
	    xmb_rt->monitors[i].f = f;
	    xmb_rt->monitors[i].arg = arg;
	    return;
	}
    }
    if (i == MAX_MONITORS) return;
    xmb_rt->monitors[i].type = type;
    xmb_rt->monitors[i].fd = fd;
    xmb_rt->monitors[i].f = f;
    xmb_rt->monitors[i].arg = arg;
    i++;
    xmb_rt->n_monitor=i;
}

void X_MB_remove_event(void *rt, int type, int fd)
{
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *) rt;
    int i;
    for(i=0;i<xmb_rt->n_monitor;i++) {
        if (xmb_rt->monitors[i].type == type && xmb_rt->monitors[i].fd == fd) {
	    xmb_rt->monitors[i].type = 0;
	    return;
	}
    }
}
mb_backend_t backend = { X_MB_new,
			 X_MB_free,
			 X_MB_add_event,
			 X_MB_remove_event,
			 X_MB_handle_connection,
			 X_MB_kbevents,
			 X_MB_rdman,
			 X_MB_tman,
			 X_MB_ob_factory,
			 X_MB_img_ldr
		};
/*! \defgroup x_supp_nodejs_sup Export functions for supporting nodejs plugin.
 *
 * These functions are for internal using.
 * @{
 */			 
/*! \brief Exported for nodejs plugin to call handle_x_event.
 */
void _X_MB_handle_x_event_for_nodejs(void *rt) {
    handle_x_event((X_MB_runtime_t *)rt);
}

/*! \brief Get X connect for nodejs plugin.
 */
int _X_MB_get_x_conn_for_nodejs(void *rt) {
    return XConnectionNumber(((X_MB_runtime_t *)rt)->display);
}

/*! \brief Flush buffer for the X connection of a runtime object.
 */
int _X_MB_flush_x_conn_for_nodejs(void *rt) {
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *)rt;
#ifdef XSHM
    XSHM_update(xmb_rt);
#endif
    return XFlush(xmb_rt->display);
}

/* @} */
