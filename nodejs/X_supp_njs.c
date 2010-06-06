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

#ifndef ASSERT
#define ASSERT(x)
#endif

typedef struct _njs_ev_data {
    ev_io iowatcher;
    ev_timer tmwatcher;
    int enable_timer;
    void *rt;
} njs_ev_data_t;

static void timer_cb(EV_P_ ev_timer *tmwatcher, int revent);

/*! \brief Register next timeout with libev.
 */
static void
set_next_timeout(njs_ev_data_t *ev_data) {
    mb_tman_t *tman;
    mb_timeval_t now, tmo;
    ev_tstamp tout;
    int r;
    
    tman = X_MB_tman(ev_data->rt);
    get_now(&now);
    r = mb_tman_next_timeout(tman, &now, &tmo);
    if(r == 0) {
	MB_TIMEVAL_DIFF(&tmo, &now);
	tout = (ev_tstamp)MB_TIMEVAL_SEC(&tmo) +
	    (ev_tstamp)MB_TIMEVAL_USEC(&tmo) / 1000000;
	ev_timer_init(&ev_data->tmwatcher, timer_cb, tout, 0);
	ev_timer_start(&ev_data->tmwatcher);
	ev_data->enable_timer = 1;
    } else
	ev_data->enable_timer = 0;
}

static void
x_conn_cb(EV_P_ ev_io *iowatcher, int revent) {
    njs_ev_data_t *ev_data = MEM2OBJ(iowatcher, njs_ev_data_t, iowatcher);
    redraw_man_t *rdman;
    extern void _X_MB_handle_x_event_for_nodejs(void *rt);

    rdman = X_MB_rdman(ev_data->rt);
    _X_MB_handle_x_event_for_nodejs(ev_data->rt);
    rdman_redraw_changed(rdman);
    
    if(ev_data->enable_timer == 0) /* no installed timeout */
	set_next_timeout(ev_data);
}

static void
timer_cb(EV_P_ ev_timer *tmwatcher, int revent) {
    njs_ev_data_t *ev_data = MEM2OBJ(tmwatcher, njs_ev_data_t, tmwatcher);
    mb_tman_t *tman;
    redraw_man_t *rdman;
    mb_timeval_t now;
    extern int _X_MB_flush_x_conn_nodejs(void *rt);
    
    tman = X_MB_tman(ev_data->rt);
    get_now(&now);
    mb_tman_handle_timeout(tman, &now);

    rdman = X_MB_rdman(ev_data->rt);
    rdman_redraw_changed(rdman);
    _X_MB_flush_x_conn_nodejs(ev_data->rt);
    
    set_next_timeout(ev_data);
}

/*! \brief Handle connection coming data and timeout of timers.
 *
 * \param rt is a runtime object for X.
 */
void
X_njs_MB_handle_connection(njs_ev_data_t *ev_data) {
    void *rt = ev_data->rt;
    mb_tman_t *tman;
    mb_timeval_t now, tmo;
    ev_tstamp tout;
    int fd;
    int r;
    extern int _X_MB_get_x_conn_for_nodejs(void *rt);

    /*
     * Setup watcher for X connection.
     */
    fd = _X_MB_get_x_conn_for_nodejs(rt);
    ev_io_init(&ev_data->iowatcher, x_conn_cb, fd, EV_READ);
    ev_io_start(&ev_data->iowatcher);

    set_next_timeout(ev_data);
}

/*! \brief Free njs_ev_data_t.
 */
void
X_njs_MB_free(njs_ev_data_t *ev_data) {
    /*
     * stop IO and timer watcher
     */
    ev_io_stop(&ev_data->iowatcher);
    if(ev_data->enable_timer)
	ev_timer_stop(&ev_data->tmwatcher);
}

njs_ev_data_t *
X_njs_MB_new(char *display_name, int w, int h) {
    njs_ev_data_t *ev_data;
    void *rt;
    
    ev_data = (njs_ev_data_t *)malloc(sizeof(njs_ev_data_t));
    ASSERT(ev_data != NULL);

    rt = X_MB_new(display_name, w, h);

    ev_data->rt = rt;
    ev_data->enable_timer = 0;	/* no timer, now */
    
    return ev_data;
}

void *
_X_njs_MB_get_runtime(nsj_ev_data_t *ev_data) {
    return ev_data->rt;
}
