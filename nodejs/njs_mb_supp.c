// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
/*! \brief Implement X11 backend for nodejs plugin.
 *
 * Since nodejs use libev to handle event loops, part of X11 backend
 * code can not be used directly.  The part of code should be rewrote.
 * The part is about
 */
#include <stdio.h>
#include <string.h>
#include <ev.h>
#include "mb_tools.h"
#include <mb_backend.h>
#include "njs_mb_supp.h"

#ifndef ASSERT
#define ASSERT(x)
#endif

#define OK 0
#define ERR -1


/*! \defgroup njs_timer_man Timer manager for nodejs.
 * @{
 */
struct _njs_timer_timeout {
    ev_timer tmwatcher;
    mb_timer_cb_t cb;
    mb_timeval_t *timeout;
    void *data;
};

static int njs_timer_man_timeout(mb_timer_man_t *tm_man,
				 mb_timeval_t *tm_out,
				 mb_timer_cb_t cb, void *data);
static void njs_timer_man_remove(mb_timer_man_t *tm_man, int tm_hdl);
static mb_timer_man_t *njs_timer_man_new(void);
static void njs_timer_man_free(mb_timer_man_t *timer_man);

static mb_timer_man_t njs_timer_man = {
    njs_timer_man_timeout,
    njs_timer_man_remove
};

static mb_timer_factory_t njs_timer_factory = {
    njs_timer_man_new,
    njs_timer_man_free
};

static void
njs_timer_man_cb(EV_P_ ev_timer *tmwatcher, int revent) {
    struct _njs_timer_timeout *timer_timeout =
	MEM2OBJ(tmwatcher, struct _njs_timer_timeout, tmwatcher);
    mb_timeval_t now;

    get_now(&now);
    timer_timeout->cb((int)timer_timeout, timer_timeout->timeout, &now,
		      timer_timeout->data);
}

static int
njs_timer_man_timeout(mb_timer_man_t *tm_man,
		      mb_timeval_t *timeout,
		      mb_timer_cb_t cb, void *data) {
    struct _njs_timer_timeout *timer_timeout;
    mb_timeval_t now, timeout_diff;
    ev_tstamp timeout_stamp;

    timer_timeout = O_ALLOC(struct _njs_timer_timeout);
    if(timer_timeout == NULL)
	return ERR;
    
    timer_timeout->cb = cb;
    timer_timeout->timeout = timeout;
    
    get_now(&now);
    
    memcpy(&timeout_diff, timeout, sizeof(mb_timeval_t));
    MB_TIMEVAL_DIFF(&timeout_diff, &now);
    timeout_stamp = (ev_tstamp)MB_TIMEVAL_SEC(&timeout_diff) +
	(ev_tstamp)MB_TIMEVAL_USEC(&timeout_diff) / 1000000;
    ev_timer_init(&timer_timeout->tmwatcher, njs_timer_man_cb,
		  timeout_stamp, 0);
    ev_timer_start(&timer_timeout->tmwatcher);

    return (int)timer_timeout;
}

static void
njs_timer_man_remove(struct _mb_timer_man *tm_man, int tm_hdl) {
    struct _njs_timer_timeout *timer_timeout =
	(struct _njs_timer_timeout *)tm_hdl;

    ev_timer_stop(&timer_timeout->tmwatcher);
    free(timer_timeout);
}

static mb_timer_man_t *
njs_timer_man_new(void) {
    return &njs_timer_man;
}

static void
njs_timer_man_free(mb_timer_man_t *timer_man) {
}

void
njs_mb_reg_timer_man(void) {
    mb_reg_timer_factory(&njs_timer_factory);
}

/* @} */


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
	_type = EV_WRITE;
    else if(type == MB_IO_RW)
	_type = EV_READ | EV_WRITE;
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
njs_mb_reg_IO_man(void) {
    mb_reg_IO_factory(&njs_io_factory);
}

/* @} */

/*! \brief Free njs_runtime_t.
 */
void
njs_mb_free(njs_runtime_t *rt) {
    /*!
     * TODO: Release all IO and timer request.
     */
    mb_runtime_free(rt->mb_rt);
    free(rt);
}

/*! \brief Free njs_runtime_t.
 */
void
njs_mb_free_keep_win(njs_runtime_t *rt) {
    /*
     * TODO: Release all IO and timer request.
     */
    mb_runtime_free_keep_win(rt->mb_rt);
    free(rt);
}

int
njs_mb_flush(njs_runtime_t *rt) {
    mb_rt_t *mb_rt = rt->mb_rt;
    int r;

    r = mb_runtime_flush(mb_rt);
    return r;
}

njs_runtime_t *
njs_mb_new(char *display_name, int w, int h) {
    njs_runtime_t *rt;
    mb_rt_t *mb_rt;

    rt = (njs_runtime_t *)malloc(sizeof(njs_runtime_t));
    ASSERT(rt != NULL);

    mb_rt = mb_runtime_new(display_name, w, h);

    rt->mb_rt = mb_rt;

    return rt;
}

/*! \brief Create a njs_runtime_t for an existed window.
 *
 * The njs_runtime_t created by this function must be free by
 * njs_mb_free_keep_win().
 */
njs_runtime_t *
njs_mb_new_with_win(void *display, long win) {
    njs_runtime_t *rt;
    mb_rt_t *mb_rt;

    rt = (njs_runtime_t *)malloc(sizeof(njs_runtime_t));
    ASSERT(rt != NULL);

    mb_rt = mb_runtime_new_with_win((Display *)display, win);

    rt->mb_rt = mb_rt;

    return rt;
}

/*! \brief Pass a X event to X runtime.
 */
void
njs_mb_handle_single_event(njs_runtime_t *rt, void *evt) {
#if 0
    void *mb_rt = rt->mb_rt;
    extern void _X_MB_handle_single_event(void *rt, void *evt);

    _X_MB_handle_single_event(mb_rt, evt);
#endif
}

/*! \brief Called at end of an iteration of event loop.
 */
void
njs_mb_no_more_event(njs_runtime_t *rt) {
#if 0
    mb_rt_t *mb_rt = rt->mb_rt;
    extern void _X_MB_no_more_event(mb_rt_t *rt);

    _X_MB_no_more_event(mb_rt);
#endif
}

/*! \brief Get X runtime that is backend of this njs runtime.
 */
mb_rt_t *
_njs_mb_get_runtime(njs_runtime_t *rt) {
    return rt->mb_rt;
}
