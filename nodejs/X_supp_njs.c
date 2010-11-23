// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
/*! \brief Implement X11 backend for nodejs plugin.
 *
 * Since nodejs use libev to handle event loops, part of X11 backend
 * code can not be used directly.  The part of code should be rewrote.
 * The part is about
 */
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <ev.h>
#include "mb_X_supp.h"
#include "mb_tools.h"
#include "X_supp_njs.h"

#ifndef ASSERT
#define ASSERT(x)
#endif

#define OK 0
#define ERR -1

static void timer_cb(EV_P_ ev_timer *tmwatcher, int revent);

/*! \brief Register next timeout with libev.
 */
static void
set_next_timeout(njs_runtime_t *rt) {
    mb_tman_t *tman;
    mb_timeval_t now, tmo;
    ev_tstamp tout;
    int r;

    tman = X_MB_tman(rt->xrt);
    get_now(&now);
    r = mb_tman_next_timeout(tman, &now, &tmo);
    if(r == 0) {
	MB_TIMEVAL_DIFF(&tmo, &now);
	tout = (ev_tstamp)MB_TIMEVAL_SEC(&tmo) +
	    (ev_tstamp)MB_TIMEVAL_USEC(&tmo) / 1000000;
	ev_timer_init(&rt->tmwatcher, timer_cb, tout, 0);
	ev_timer_start(&rt->tmwatcher);
	rt->enable_timer = 1;
    } else
	rt->enable_timer = 0;
}

static void
x_conn_cb(EV_P_ ev_io *iowatcher, int revent) {
    njs_runtime_t *rt = MEM2OBJ(iowatcher, njs_runtime_t, iowatcher);
    redraw_man_t *rdman;
    extern void _X_MB_handle_x_event_for_nodejs(void *rt);

    rdman = X_MB_rdman(rt->xrt);
    _X_MB_handle_x_event_for_nodejs(rt->xrt);
    rdman_redraw_changed(rdman);

    if(rt->enable_timer == 0) /* no installed timeout */
	set_next_timeout(rt);
}

static void
timer_cb(EV_P_ ev_timer *tmwatcher, int revent) {
    njs_runtime_t *rt = MEM2OBJ(tmwatcher, njs_runtime_t, tmwatcher);
    mb_tman_t *tman;
    redraw_man_t *rdman;
    mb_timeval_t now;
    extern int _X_MB_flush_x_conn_for_nodejs(void *rt);

    tman = X_MB_tman(rt->xrt);
    get_now(&now);
    mb_tman_handle_timeout(tman, &now);

    rdman = X_MB_rdman(rt->xrt);
    rdman_redraw_changed(rdman);
    _X_MB_flush_x_conn_for_nodejs(rt->xrt);

    set_next_timeout(rt);
}

/*! \defgroup njs_io_man IO manager for nodejs.
 * @{
 */
struct _njs_io_reg {
    ev_io iowatcher;
    int fd;
    mb_IO_cb_t cb;
    void *data;
};

static int njs_io_man_reg(struct _mb_IO_man *io_man,
			  int fd, MB_IO_TYPE type, mb_IO_cb_t cb, void *data);
static void njs_io_man_unreg(struct _mb_IO_man *io_man, int io_hdl);
static mb_IO_man_t *njs_io_man_new(void);
static void njs_io_man_free(mb_IO_man_t *io_man);

static mb_IO_man_t njs_io_man = {
    njs_io_man_reg,
    njs_io_man_unreg
};

/*! \brief IO factory to integrate MadButterfly to event loop of nodejs.
 */
static mb_IO_factory_t njs_io_factory = {
    njs_io_man_new,
    njs_io_man_free
};

/*! \brief Bridge libev callback to IO manager callback.
 */
static void
njs_io_man_cb(EV_P_ ev_io *iowatcher, int revent) {
    struct _njs_io_reg *io_reg =
	MEM2OBJ(iowatcher, struct _njs_io_reg, iowatcher);
    MB_IO_TYPE type;

    switch(revent & (EV_READ | EV_WRITE)) {
    case EV_READ:
	type = MB_IO_R;
	break;
    case EV_WRITE:
	type = MB_IO_W;
	break;
    case EV_READ | EV_WRITE:
	type = MB_IO_RW;
	break;
    }
    
    io_reg->cb((int)io_reg, io_reg->fd, type, io_reg->data);
}

static int
njs_io_man_reg(struct _mb_IO_man *io_man,
	       int fd, MB_IO_TYPE type, mb_IO_cb_t cb, void *data) {
    int _type;
    struct _njs_io_reg *io_reg;

    if(type == MB_IO_R)
	_type = EV_READ;
    else if(type == MB_IO_W)
	_type == EV_WRITE;
    else if(type == MB_IO_RW)
	_type == EV_READ | EV_WRITE;
    else
	return ERR;
    
    io_reg = O_ALLOC(struct _njs_io_reg);
    if(io_reg == NULL)
	return ERR;
    
    io_reg->fd = fd;
    io_reg->cb = cb;
    io_reg->data = data;

    ev_io_init(&io_reg->iowatcher, njs_io_man_cb, fd, _type);
    ev_io_start(&io_reg->iowatcher);
    
    return (int)io_reg;
}

static void
njs_io_man_unreg(struct _mb_IO_man *io_man, int io_hdl) {
    struct _njs_io_reg *io_reg = (struct _njs_io_reg *)io_hdl;

    ev_io_stop(&io_reg->iowatcher);
    free(io_reg);
}

static mb_IO_man_t *
njs_io_man_new(void) {
    return &njs_io_man;
}

static void
njs_io_man_free(mb_IO_man_t *io_man) {
}

/*! \brief Register an IO factory with MadButterfly backend.
 */
void
X_njs_MB_reg_IO_man(void) {
    mb_reg_IO_facotry(&njs_io_factory);
}

/* @} */

/*! \brief Handle connection coming data and timeout of timers.
 *
 * \param rt is a runtime object for X.
 */
void
X_njs_MB_init_handle_connection(njs_runtime_t *rt) {
    void *xrt = rt->xrt;
    int fd;
    extern int _X_MB_get_x_conn_for_nodejs(void *rt);

    /*
     * Setup watcher for X connection.
     */
    fd = _X_MB_get_x_conn_for_nodejs(xrt);
    ev_io_init(&rt->iowatcher, x_conn_cb, fd, EV_READ);
    ev_io_start(&rt->iowatcher);
    rt->enable_io = 1;

    set_next_timeout(rt);
}

/*! \brief Free njs_runtime_t.
 */
void
X_njs_MB_free(njs_runtime_t *rt) {
    /*
     * stop IO and timer watcher
     */
    if(rt->enable_io)
	ev_io_stop(&rt->iowatcher);
    if(rt->enable_timer)
	ev_timer_stop(&rt->tmwatcher);

    X_MB_free(rt->xrt);
    free(rt);
}

/*! \brief Free njs_runtime_t.
 */
void
X_njs_MB_free_keep_win(njs_runtime_t *rt) {
    /*
     * stop IO and timer watcher
     */
    if(rt->enable_io)
	ev_io_stop(&rt->iowatcher);
    if(rt->enable_timer)
	ev_timer_stop(&rt->tmwatcher);

    X_MB_free_keep_win(rt->xrt);
    free(rt);
}

int
X_njs_MB_flush(njs_runtime_t *rt) {
    void *xrt = rt->xrt;
    int r;
    extern int _X_MB_flush_x_conn_for_nodejs(void *rt);

    _X_MB_flush_x_conn_for_nodejs(xrt);

    return r;
}

njs_runtime_t *
X_njs_MB_new(char *display_name, int w, int h) {
    njs_runtime_t *rt;
    void *xrt;

    rt = (njs_runtime_t *)malloc(sizeof(njs_runtime_t));
    ASSERT(rt != NULL);

    xrt = X_MB_new(display_name, w, h);

    rt->xrt = xrt;
    rt->enable_io = 0;
    rt->enable_timer = 0;	/* no timer, now */

    return rt;
}

/*! \brief Create a njs_runtime_t for an existed window.
 *
 * The njs_runtime_t created by this function must be free by
 * X_njs_MB_free_keep_win().
 */
njs_runtime_t *
X_njs_MB_new_with_win(void *display, long win) {
    njs_runtime_t *rt;
    void *xrt;

    rt = (njs_runtime_t *)malloc(sizeof(njs_runtime_t));
    ASSERT(rt != NULL);

    xrt = X_MB_new_with_win((Display *)display, win);

    rt->xrt = xrt;
    rt->enable_io = 0;
    rt->enable_timer = 0;	/* no timer, now */

    return rt;
}

/*! \brief Pass a X event to X runtime.
 */
void
X_njs_MB_handle_single_event(njs_runtime_t *rt, void *evt) {
    void *xrt = rt->xrt;
    extern void _X_MB_handle_single_event(void *rt, void *evt);

    _X_MB_handle_single_event(xrt, evt);
}

/*! \brief Called at end of an iteration of event loop.
 */
void
X_njs_MB_no_more_event(njs_runtime_t *rt) {
    mb_rt_t *xrt = rt->xrt;
    extern void _X_MB_no_more_event(mb_rt_t *rt);

    _X_MB_no_more_event(xrt);
}

/*! \brief Get X runtime that is backend of this njs runtime.
 */
mb_rt_t *
_X_njs_MB_get_X_runtime(njs_runtime_t *rt) {
    return rt->xrt;
}
