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
    void *xrt = rt->xrt;

    _X_MB_no_more_event(xrt);
}

/*! \brief Get X runtime that is backend of this njs runtime.
 */
void *
_X_njs_MB_get_X_runtime(njs_runtime_t *rt) {
    return rt->xrt;
}
