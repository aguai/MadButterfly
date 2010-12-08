// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include "mb_graph_engine.h"
#include "mb_redraw_man.h"
#include "mb_timer.h"
#include "mb_cons_supp.h"
#include "mb_backend.h"
#include "mb_backend_utils.h"
#include "config.h"

#define ERR -1
#define OK 0

#define FALSE 0
#define TRUE 1

#define ASSERT(x)

#define ONLY_MOUSE_MOVE_RAW 1

typedef int keysym;

static mb_timer_factory_t *_timer_factory = &tman_timer_factory;

/*! \ingroup console_kb
 * @{
 */
struct _cons_kb_info {
    int kb_fd;
    
    int keycode_min, keycode_max;
    int ksym_per_code;
    keysym *syms;
    subject_t *kbevents;
    observer_factory_t *observer_factory;
};
typedef struct _cons_kb_info cons_kb_info_t;

/* @} */

struct _cons_supp_runtime {
    MB_DISPLAY display;
    
    mbe_surface_t *surface;
    mbe_t *cr;
    redraw_man_t *rdman;
    mb_img_ldr_t *img_ldr;
    int w, h;

    cons_kb_info_t kbinfo;
    mb_IO_man_t *io_man;
    mb_timer_man_t *timer_man;

#ifndef ONLY_MOUSE_MOVE_RAW
    /* States */
    shape_t *last;
#endif

    /* For handle connection */
    int io_hdl;

    /*
     * Following variables are used by handle_single_cons_event()
     */
    int last_evt_type;	       /* Type of last event */
    int ex1, ey1, ex2, ey2;    /* Aggregate expose events */
    int mx, my;		       /* Position of last motion event */
    int mbut_state;	       /* Button state of last motion event */
};
typedef struct _cons_supp_runtime cons_supp_runtime_t;

static void _cons_supp_handle_cons_event(cons_supp_runtime_t *rt);

/*! \defgroup cons_supp_io IO manager for console.
 * @{
 */
#define MAX_MONITORS 200

typedef struct {
    int type;
    int fd;
    mb_IO_cb_t cb;
    void *data;
}  monitor_t;

struct _cons_supp_IO_man {
    mb_IO_man_t io_man;
    monitor_t monitors[MAX_MONITORS];
    int n_monitor;
};

static int _cons_supp_io_man_reg(struct _mb_IO_man *io_man,
				int fd, MB_IO_TYPE type,
				mb_IO_cb_t cb, void *data);
static void _cons_supp_io_man_unreg(struct _mb_IO_man *io_man, int io_hdl);
static mb_IO_man_t *_cons_supp_io_man_new(void);
static void _cons_supp_io_man_free(mb_IO_man_t *io_man);

static mb_IO_factory_t _cons_supp_default_io_factory = {
    _cons_supp_io_man_new,
    _cons_supp_io_man_free
};
static mb_IO_factory_t *_io_factory = &_cons_supp_default_io_factory;

static struct _cons_supp_IO_man _default_io_man = {
    {_cons_supp_io_man_reg, _cons_supp_io_man_unreg},
    {},			/* monitors */
    0			/* n_monitor */
};

static mb_IO_man_t *
_cons_supp_io_man_new(void) {
    return (mb_IO_man_t *)&_default_io_man;
}

static void
_cons_supp_io_man_free(mb_IO_man_t *io_man) {
}

static int
_cons_supp_io_man_reg(struct _mb_IO_man *io_man,
		     int fd, MB_IO_TYPE type, mb_IO_cb_t cb, void *data) {
    struct _cons_supp_IO_man *cmb_io_man = (struct _cons_supp_IO_man *)io_man;
    int i;

    for(i = 0; i < cmb_io_man->n_monitor; i++) {
        if (cmb_io_man->monitors[i].type == MB_IO_DUMMY)
	    break;
    }
    if (i == MAX_MONITORS)
	return ERR;
    
    cmb_io_man->monitors[i].type = type;
    cmb_io_man->monitors[i].fd = fd;
    cmb_io_man->monitors[i].cb = cb;
    cmb_io_man->monitors[i].data = data;
    i++;
    if(i > cmb_io_man->n_monitor)
	cmb_io_man->n_monitor = i;
    return i - 1;
}

static void
_cons_supp_io_man_unreg(struct _mb_IO_man *io_man, int io_hdl) {
    struct _cons_supp_IO_man *cmb_io_man = (struct _cons_supp_IO_man *)io_man;
    
    ASSERT(io_hdl < cmb_io_man->n_monitor);
    cmb_io_man->monitors[io_hdl].type = MB_IO_DUMMY;
}

/*! \brief Handle connection coming data and timeout of timers.
 *
 */
static void
_cons_supp_event_loop(mb_rt_t *rt) {
    struct _cons_supp_runtime *cmb_rt = (struct _cons_supp_runtime *)rt;
    struct _cons_supp_IO_man *io_man =
	(struct _cons_supp_IO_man *)cmb_rt->io_man;
    mb_timer_man_t *timer_man = (mb_timer_man_t *)cmb_rt->timer_man;
    redraw_man_t *rdman;
    mb_tman_t *tman = tman_timer_man_get_tman(timer_man);
    mb_timeval_t now, tmo;
    struct timeval tv;
    fd_set rfds, wfds;
    int nfds = 0;
    int r, r1,i;

    rdman = mb_runtime_rdman(rt);
    rdman_redraw_all(rdman);
    
    _cons_supp_handle_cons_event(cmb_rt);

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

/*! \defgroup console_kb Console Keyboard Handling
 *
 * Accept keyboard events from console and delivery it to
 * application through observer pattern.  There is a subject,
 * per X-connection, for that.
 * @{
 */
static int keycode2sym(cons_kb_info_t *kbinfo, unsigned int keycode) {
    /* TODO: implement keycode to key symbol translation */
    return 0;
}

static int cons_kb_init(cons_kb_info_t *kbinfo, MB_DISPLAY display,
		       redraw_man_t *rdman) {
    int n_syms;
    observer_factory_t *factory;
    int r;

    /* TODO: set keycode_min, keycode_max and syms */
    if((int)display != -1)
	kbinfo->kb_fd = (int)display;
    else
	kbinfo->kb_fd = STDIN_FILENO;

    factory = rdman_get_observer_factory(rdman);
    kbinfo->kbevents = subject_new(factory, kbinfo, OBJT_KB);
    if(kbinfo->kbevents == NULL)
	return ERR;
    /*! \todo Make sure observer_factory is still need. */
    kbinfo->observer_factory = factory;

    return OK;
}

static void cons_kb_destroy(cons_kb_info_t *kbinfo) {
    subject_free(kbinfo->kbevents);
}
/* @} */

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

/*! \brief Handle keyboard event and maintain internal states.
 *
 * It keeps internal state in rt to improve performance.
 */
static void
handle_single_cons_event(cons_supp_runtime_t *rt) {
    /* TODO: handle keyboard and mouse events. */
    printf("handle_single_cons_event() will be implemented later\n");
}

/*! \brief Call when no more event in an event iteration.
 *
 * No more event means event queue is emplty.  This function will
 * perform some actions according current internal state.
 */
static void
no_more_event(cons_supp_runtime_t *rt) {
}

/*! \brief Dispatch all console events in the queue.
 */
static void _cons_supp_handle_cons_event(cons_supp_runtime_t *cmb_rt) {
    int console_fd = (int)cmb_rt->display;
    struct pollfd pfd = {console_fd, POLLIN, 0};
    int r;

    while((r = poll(&pfd, 1, 0)) > 0) {
	handle_single_cons_event(cmb_rt);
    }
    no_more_event(cmb_rt);
}

static void
_cons_supp_handle_connection(int hdl, int fd, MB_IO_TYPE type, void *data) {
    cons_supp_runtime_t *cmb_rt = (cons_supp_runtime_t *)data;

    _cons_supp_handle_cons_event(cmb_rt);
}

/*! \brief Initialize a MadButterfy runtime for Xlib.
 *
 * This one is very like _cons_supp_init(), except it accepts a
 * cons_supp_runtime_t object initialized with a display connected to a X
 * server and an opened window.
 *
 * Following field of the cons_supp_runtime_t object should be initialized.
 *   - w, h
 *   - win
 *   - display
 *   - visual
 */
static int
_cons_supp_init_with_win_internal(cons_supp_runtime_t *cmb_rt) {
    mb_img_ldr_t *img_ldr;
    int w, h;
    int console_fd;

    w = cmb_rt->w;
    h = cmb_rt->h;
    
    mbe_init();
    
    cmb_rt->surface =
	mbe_win_surface_create(cmb_rt->display, NULL,
			       MB_IFMT_ARGB32, w, h);

    cmb_rt->cr = mbe_create(cmb_rt->surface);
    
    cmb_rt->rdman = (redraw_man_t *)malloc(sizeof(redraw_man_t));
    redraw_man_init(cmb_rt->rdman, cmb_rt->cr, NULL);
    cmb_rt->rdman->w = w;
    cmb_rt->rdman->h = h;
    /* FIXME: This is a wired loopback reference. This is inly
     *        required when we need to get the cmb_rt->tman for the
     *        animation. We should relocate the tman to the
     *        redraw_man_t instead.
     */
    cmb_rt->rdman->rt = cmb_rt;

    cmb_rt->io_man = mb_io_man_new(_io_factory);
    cmb_rt->timer_man = mb_timer_man_new(_timer_factory);

    img_ldr = simple_mb_img_ldr_new("");
    cmb_rt->img_ldr = img_ldr;
    /*! \todo Remove rdman_set_img_ldr() */
    rdman_set_img_ldr(cmb_rt->rdman, img_ldr); /* this is ncessary? */

#ifndef ONLY_MOUSE_MOVE_RAW
    cmb_rt->last = NULL;
#endif

    cons_kb_init(&cmb_rt->kbinfo, cmb_rt->display, cmb_rt->rdman);

    console_fd = (int)cmb_rt->display;
    cmb_rt->io_hdl = mb_io_man_reg(cmb_rt->io_man, console_fd,
				   MB_IO_R,
				   _cons_supp_handle_connection,
				   cmb_rt);

    return OK;
}

/*! \brief Initialize a MadButterfy runtime for console.
 *
 * It setups a runtime environment to run MadButterfly with console.
 * Users should specify width and height of the opening window.
 *
 * \param display_name is actually the path to the console/input device.
 */
static int _cons_supp_init(cons_supp_runtime_t *cmb_rt,
			  const char *display_name,
			  int w, int h) {
    int r;
    int console_fd;

    memset(cmb_rt, 0, sizeof(cons_supp_runtime_t));

    if(display_name == NULL || strlen(display_name) == 0)
	console_fd = STDIN_FILENO;
    else {
	console_fd = open(display_name, O_RDONLY);
	if(console_fd == -1)
	    return ERR;
    }
    
    cmb_rt->display = (MB_DISPLAY)console_fd;
    cmb_rt->w = w;
    cmb_rt->h = h;
    
    r = _cons_supp_init_with_win_internal(cmb_rt);
    
    return r;
}

/*! \brief Initialize a MadButterfly runtime for a window of console.
 *
 * This function is equivalent to _cons_supp_init() with fixed width
 * and height.  Since, there is no window for console.
 *
 * Runtimes initialized with this function should be destroyed with
 * cons_supp_destroy_keep_win().
 *
 * \param display is actually a file descriptor of console (input device).
 */
static int
_cons_supp_init_with_win(cons_supp_runtime_t *cmb_rt,
			MB_DISPLAY display, MB_WINDOW win) {
    int r;

    memset(cmb_rt, 0, sizeof(cons_supp_runtime_t));

    cmb_rt->display = display;
    cmb_rt->w = 800;
    cmb_rt->h = 600;
    
    r = _cons_supp_init_with_win_internal(cmb_rt);

    return r;
}

static void cons_supp_destroy_keep_win(cons_supp_runtime_t *cmb_rt);

static void cons_supp_destroy(cons_supp_runtime_t *cmb_rt) {
    int console_fd = cmb_rt = (int)cmb_rt->display;
    
    close(console_fd);
    cons_supp_destroy_keep_win(cmb_rt);
}

/*! \brief Destroy a MadButterfly runtime initialized with
 *	_cons_supp_init_with_win().
 *
 * Destroying a runtime with this function prevent the window and
 * display associated with the runtime being closed.
 */
static void
cons_supp_destroy_keep_win(cons_supp_runtime_t *cmb_rt) {
    if(cmb_rt->rdman) {
	redraw_man_destroy(cmb_rt->rdman);
	free(cmb_rt->rdman);
    }

    if(cmb_rt->io_hdl)
	mb_io_man_unreg(cmb_rt->io_man, cmb_rt->io_hdl);

    if(cmb_rt->io_man)
	mb_io_man_free(_io_factory, cmb_rt->io_man);
    if(cmb_rt->timer_man)
	mb_timer_man_free(_timer_factory, cmb_rt->timer_man);

    if(cmb_rt->img_ldr)
	MB_IMG_LDR_FREE(cmb_rt->img_ldr);

    if(cmb_rt->cr)
	mbe_destroy(cmb_rt->cr);

    if(cmb_rt->surface)
	mbe_surface_destroy(cmb_rt->surface);

    cons_kb_destroy(&cmb_rt->kbinfo);
}

static mb_rt_t *
_cons_supp_new(const char *display_name, int w, int h) {
    cons_supp_runtime_t *rt;
    int r;

    rt = O_ALLOC(cons_supp_runtime_t);
    if(rt == NULL)
	return NULL;

    r = _cons_supp_init(rt, display_name, w, h);
    if(r != OK) {
	free(rt);
	return NULL;
    }

    return (mb_rt_t *)rt;
}

/*! \brief Create a new runtime for existed window for X.
 *
 * The object returned by this function must be free with
 * _cons_supp_free_keep_win() to prevent the window from closed.
 */
static mb_rt_t *
_cons_supp_new_with_win(MB_DISPLAY display, MB_WINDOW win) {
    cons_supp_runtime_t *rt;
    int r;

    rt = O_ALLOC(cons_supp_runtime_t);
    if(rt == NULL)
	return NULL;

    r = _cons_supp_init_with_win(rt, display, win);
    if(r != OK) {
	free(rt);
	return NULL;
    }

    return (mb_rt_t *)rt;
}

static void
_cons_supp_free(mb_rt_t *rt) {
    cons_supp_destroy((cons_supp_runtime_t *) rt);
    free(rt);
}

/*! \brief Free runtime created with _cons_supp_new_with_win().
 */
static void
_cons_supp_free_keep_win(mb_rt_t *rt) {
    cons_supp_destroy_keep_win((cons_supp_runtime_t *) rt);
    free(rt);
}

static subject_t *
_cons_supp_kbevents(mb_rt_t *rt) {
    cons_supp_runtime_t *cmb_rt = (cons_supp_runtime_t *) rt;
    return cmb_rt->kbinfo.kbevents;
}

static redraw_man_t *
_cons_supp_rdman(mb_rt_t *rt) {
    cons_supp_runtime_t *cmb_rt = (cons_supp_runtime_t *) rt;
    return cmb_rt->rdman;
}

static mb_timer_man_t *
_cons_supp_timer_man(mb_rt_t *rt) {
    cons_supp_runtime_t *cmb_rt = (cons_supp_runtime_t *) rt;
    return cmb_rt->timer_man;
}

static observer_factory_t *
_cons_supp_observer_factory(mb_rt_t *rt) {
    cons_supp_runtime_t *cmb_rt = (cons_supp_runtime_t *) rt;
    observer_factory_t *factory;

    factory = rdman_get_observer_factory(cmb_rt->rdman);
    return factory;
}

static mb_img_ldr_t *
_cons_supp_img_ldr(mb_rt_t *rt) {
    cons_supp_runtime_t *cmb_rt = (cons_supp_runtime_t *) rt;
    mb_img_ldr_t *img_ldr;

    img_ldr = cmb_rt->img_ldr;

    return img_ldr;
}

static int
_cons_supp_add_event(mb_rt_t *rt, int fd, MB_IO_TYPE type,
		    mb_IO_cb_t cb, void *data)
{
    cons_supp_runtime_t *cmb_rt = (cons_supp_runtime_t *) rt;
    mb_IO_man_t *io_man = cmb_rt->io_man;
    int hdl;

    hdl = mb_io_man_reg(io_man, fd, type, cb, data);
    return hdl;
}

static void
_cons_supp_remove_event(mb_rt_t *rt, int hdl)
{
    cons_supp_runtime_t *cmb_rt = (cons_supp_runtime_t *) rt;
    mb_IO_man_t *io_man = cmb_rt->io_man;

    mb_io_man_unreg(io_man, hdl);
}

static int
_cons_supp_flush(mb_rt_t *rt) {
    cons_supp_runtime_t *cmb_rt = (cons_supp_runtime_t *)rt;
    
    mbe_flush(cmb_rt->cr);
    return OK;
}

static void
_cons_supp_reg_IO_factory(mb_IO_factory_t *io_factory) {
    _io_factory = io_factory;
}

static void
_cons_supp_reg_timer_factory(mb_timer_factory_t *timer_factory) {
    _timer_factory = timer_factory;
}

mb_backend_t mb_dfl_backend = { _cons_supp_new,
				_cons_supp_new_with_win,
				
				_cons_supp_free,
				_cons_supp_free_keep_win,
				_cons_supp_add_event,
				_cons_supp_remove_event,
				_cons_supp_event_loop,
				_cons_supp_flush,
				
				_cons_supp_kbevents,
				_cons_supp_rdman,
				_cons_supp_timer_man,
				_cons_supp_observer_factory,
				_cons_supp_img_ldr,

				_cons_supp_reg_IO_factory,
				_cons_supp_reg_timer_factory,
};
