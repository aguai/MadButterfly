// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <cairo/cairo.h>
#include <cairo-directfb.h>
#include "mb_graph_engine.h"
#include "mb_redraw_man.h"
#include "mb_timer.h"
#include "mb_dfb_supp.h"
#include "config.h"

#define ERR -1
#define OK 0

#define ONLY_MOUSE_MOVE_RAW 1

/*! \ingroup xkb
 * @{
 */
struct _X_kb_info {
    int keycode_min, keycode_max;
    int ksym_per_code;
    subject_t *kbevents;
    observer_factory_t *observer_factory;
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
    IDirectFB *dfb;
    IDirectFBSurface *primary;
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
/*    sym =  kbinfo->syms[sym_idx];*/
    return sym;
}

static void X_kb_destroy(X_kb_info_t *kbinfo) {
    subject_free(kbinfo->kbevents);
}

/* @} */

static unsigned int get_button_state(unsigned int state) {
    return 0;
}

static unsigned int get_button(unsigned int button) {
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
handle_motion_event(X_MB_runtime_t *rt) {
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
handle_expose_event(X_MB_runtime_t *rt) {
    redraw_man_t *rdman = rt->rdman;
    int ex1, ey1, ex2, ey2;

    ex1 = rt->ex1;
    ey1 = rt->ey1;
    ex2 = rt->ex2;
    ey2 = rt->ey2;

    rdman_redraw_area(rdman, ex1, ey1, (ex2 - ex1), (ey2 - ey1));

    rt->eflag = 0;
}

/*! \brief Call when no more event in an event iteration.
 *
 * No more event means event queue is emplty.  This function will
 * perform some actions according current internal state.
 */
static void
no_more_event(X_MB_runtime_t *rt) {
    if(rt->mflag)
	handle_motion_event(rt);
    if(rt->eflag)
	handle_expose_event(rt);
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
    redraw_man_t *rdman = rt->rdman;
    mb_tman_t *tman = rt->tman;
    int fd = 0;
    mb_timeval_t now, tmo;
    struct timeval tv;
    fd_set rfds,wfds;
    int nfds;
    int r, r1,i;

    handle_x_event(rt);

/*    fd = XConnectionNumber(display);*/
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

static int dfb_init_connection(int w, int h,
			     IDirectFB **dfb,
			     IDirectFBSurface **primary) {
    DFBSurfaceDescription dsc;

    DirectFBInit(NULL, NULL);
    DirectFBCreate(dfb);
    (*dfb)->SetCooperativeLevel(*dfb, DFSCL_FULLSCREEN);
    dsc.flags = DSDESC_CAPS;
    dsc.caps  = DSCAPS_PRIMARY;
    (*dfb)->CreateSurface(*dfb, &dsc, primary);
    (*primary)->GetSize(*primary, &w, &h);
    (*primary)->SetColor(*primary, 0xff, 0xff, 0xff, 0xff);
    (*primary)->FillRectangle(*primary, 0, 0, w, h);

    return OK;
}

/*! \brief Initialize a MadButterfy runtime for DirectFB.
 *
 * This one is very like X_MB_init(), except it accepts a
 * X_MB_runtime_t object initialized with a display connected to a DirectFB
 * server and an opened window.
 *
 * Following field of the X_MB_runtime_t object should be initialized.
 *   - w, h
 *   - win
 *   - dfb
 *   - primary
 */
static int X_MB_init_with_win_internal(X_MB_runtime_t *xmb_rt) {
    mb_img_ldr_t *img_ldr;
    int w, h;

    w = xmb_rt->w;
    h = xmb_rt->h;

    xmb_rt->surface =
	mbe_image_surface_create(MB_IFMT_ARGB32, w, h);

    xmb_rt->surface_ptn =
	mbe_pattern_create_for_surface(xmb_rt->surface);

    if (xmb_rt->backend_surface == NULL)
	xmb_rt->backend_surface =
	    mbe_directfb_surface_create(xmb_rt->dfb, xmb_rt->primary);

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

/*    X_kb_init(&xmb_rt->kbinfo, xmb_rt->display, xmb_rt->rdman);*/

    return OK;
}

/*! \brief Initialize a MadButterfy runtime for DirectFB.
 *
 * It setups a runtime environment to run MadButterfly with DirectFB.
 * Users should specify width and height of the opening window.
 */
static int X_MB_init(X_MB_runtime_t *xmb_rt, const char *display_name,
		     int w, int h) {
    int r;

    memset(xmb_rt, 0, sizeof(X_MB_runtime_t));

    xmb_rt->w = w;
    xmb_rt->h = h;
    r = dfb_init_connection(w, h, &xmb_rt->dfb, &xmb_rt->primary);

    if(r != OK)
	return ERR;

    r = X_MB_init_with_win_internal(xmb_rt);

    return r;
}

static void X_MB_destroy(X_MB_runtime_t *xmb_rt) {
    if (xmb_rt->rdman) {
	redraw_man_destroy(xmb_rt->rdman);
	free(xmb_rt->rdman);
    }

    if (xmb_rt->tman)
	mb_tman_free(xmb_rt->tman);

    if (xmb_rt->img_ldr)
	MB_IMG_LDR_FREE(xmb_rt->img_ldr);

    if (xmb_rt->cr)
	mbe_destroy(xmb_rt->cr);

    if (xmb_rt->backend_cr)
	mbe_destroy(xmb_rt->backend_cr);

    if(xmb_rt->surface)
	mbe_surface_destroy(xmb_rt->surface);

    if(xmb_rt->surface_ptn)
	mbe_pattern_destroy(xmb_rt->surface_ptn);

    if(xmb_rt->backend_surface)
	mbe_surface_destroy(xmb_rt->backend_surface);

    if (xmb_rt->primary)
	xmb_rt->primary->Release(xmb_rt->primary);

    if (xmb_rt->dfb)
	xmb_rt->dfb->Release(xmb_rt->dfb);
}

void *X_MB_new(const char *display_name, int w, int h) {
    X_MB_runtime_t *rt;
    int r;

    rt = O_ALLOC(X_MB_runtime_t);

    if (rt == NULL)
	return NULL;

    r = X_MB_init(rt, display_name, w, h);

    if (r != OK) {
	free(rt);
	return NULL;
    }

    return rt;
}

void X_MB_free(void *rt) {
    X_MB_destroy((X_MB_runtime_t *) rt);
    free(rt);
}

/*! \brief Free runtime created with X_MB_new_with_win().
 */
void
X_MB_free_keep_win(void *rt) {
    X_MB_destroy_keep_win((X_MB_runtime_t *) rt);
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

observer_factory_t *X_MB_observer_factory(void *rt) {
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *) rt;
    observer_factory_t *factory;

    factory = rdman_get_observer_factory(xmb_rt->rdman);
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
			 X_MB_observer_factory,
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

/*! \brief Called at end of an iteration of X event loop.
 */
void
_X_MB_no_more_event(void *rt) {
    X_MB_runtime_t *xmb_rt = (X_MB_runtime_t *)rt;

    no_more_event(xmb_rt);
}

/* @} */
