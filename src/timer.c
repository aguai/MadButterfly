// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mb_timer.h"
#include "mb_tools.h"
#include "mb_backend.h"


#define OK 0
#define ERR -1

struct _mb_timer {
    mb_timeval_t tmo;
    mb_tmo_hdlr hdlr;
    void *arg;
    mb_timer_t *next;
};

struct _mb_tman {
    STAILQ(mb_timer_t) timers;
    elmpool_t *timer_pool;
};

mb_tman_t *mb_tman_new(void) {
    mb_tman_t *tman;

    tman = (mb_tman_t *)malloc(sizeof(mb_tman_t));
    if(tman == NULL)
	return NULL;

    tman->timer_pool = elmpool_new(sizeof(mb_timer_t), 32);
    if(tman->timer_pool == NULL) {
	free(tman);
	return NULL;
    }

    STAILQ_INIT(tman->timers);

    return tman;
}

void mb_tman_free(mb_tman_t *tman) {
    elmpool_free(tman->timer_pool);
    free(tman);
}

mb_timer_t *mb_tman_timeout(mb_tman_t *tman,
			    const mb_timeval_t *tmo,
			    mb_tmo_hdlr hdlr, void *arg) {
    mb_timer_t *timer, *visit, *last;

    timer = elmpool_elm_alloc(tman->timer_pool);
    if(timer == NULL)
	return NULL;

    MB_TIMEVAL_CP(&timer->tmo, tmo);
    timer->hdlr = hdlr;
    timer->arg = arg;

    last = NULL;
    for(visit = STAILQ_HEAD(tman->timers);
	visit != NULL;
	visit = STAILQ_NEXT(mb_timer_t, next, visit)) {
	if(MB_TIMEVAL_LATER(&visit->tmo, tmo) ||
	   MB_TIMEVAL_EQ(&visit->tmo, tmo))
	    break;
	last = visit;
    }

    if(last == NULL)
	STAILQ_INS(tman->timers, mb_timer_t, next, timer);
    else if(visit == NULL)
	STAILQ_INS_TAIL(tman->timers, mb_timer_t, next, timer);
    else
	STAILQ_INS_AFTER(mb_timer_t, next, timer, last);

    return timer;
}

int mb_tman_remove(mb_tman_t *tman, mb_timer_t *timer) {
    STAILQ_REMOVE(tman->timers, mb_timer_t, next, timer);
    elmpool_elm_free(tman->timer_pool, timer);

    return OK;
}

/*! \brief Get how long to next timeout from this monent.
 *
 * \return 0 for having next timeout, -1 for not more timeout.
 */
int mb_tman_next_timeout(mb_tman_t *tman,
			 const mb_timeval_t *now, mb_timeval_t *tmo_after) {
    mb_timer_t *timer;

    timer = STAILQ_HEAD(tman->timers);
    if(timer == NULL)
	return ERR;

    if(!MB_TIMEVAL_LATER(&timer->tmo, now)) {
	memset(tmo_after, 0, sizeof(mb_timeval_t));
	return OK;
    }

    MB_TIMEVAL_CP(tmo_after, &timer->tmo);
    MB_TIMEVAL_DIFF(tmo_after, now);

    return OK;
}

int mb_tman_handle_timeout(mb_tman_t *tman, mb_timeval_t *now) {
    mb_timer_t *timer;

    while((timer = STAILQ_HEAD(tman->timers)) != NULL){
	if(MB_TIMEVAL_LATER(&timer->tmo, now))
	    break;
	timer->hdlr(&timer->tmo, now, timer->arg);
	STAILQ_REMOVE(tman->timers, mb_timer_t, next, timer);
	elmpool_elm_free(tman->timer_pool, timer);
    }

    return OK;
}

/*! \defgroup tman_timer_man Timer manager based on mb_tman_t.
 *
 * This implmentation of timer manager is based on mb_tman_t.
 * @{
 */
struct _tman_timer_man {
    mb_timer_man_t timer_man;
    mb_tman_t *tman;
};

static int _tman_timer_man_timeout(struct _mb_timer_man *tm_man,
				   mb_timeval_t *tmout,
				   mb_timer_cb_t cb, void *data);
static void _tman_timer_man_remove(struct _mb_timer_man *tm_man, int tm_hdl);
static mb_timer_man_t *_tman_timer_fact_new(void);
static void _tman_timer_fact_free(mb_timer_man_t *timer_man);

static struct _tman_timer_man _tman_default_timer_man = {
    {_tman_timer_man_timeout, _tman_timer_man_remove},
    NULL
};

mb_timer_factory_t tman_timer_factory = {
    _tman_timer_fact_new,
    _tman_timer_fact_free
};

/*! \brief Content of a timeout request.
 *
 * This is only used by internal of X support.  This data structure
 * carry information to adopt mb_tman_t to mb_timer_man_t.
 */
struct _tman_timeout_data {
    mb_timer_t *timer;		/*!< Handle returned by mb_tman_timeout() */
    mb_timer_cb_t cb;		/*!< Real callback function */
    void *data;			/*!< data for real callback */
};

static void
_tman_tmo_hdlr(const mb_timeval_t *tmo,
		 const mb_timeval_t *now,
		 void *arg) {
    struct _tman_timeout_data *data = (struct _tman_timeout_data *)arg;
    
    data->cb((int)data, tmo, now, data->data);
}

static int
_tman_timer_man_timeout(struct _mb_timer_man *tm_man,
			  mb_timeval_t *tmout, /* timeout (wall time) */
			  mb_timer_cb_t cb, void *data) {
    struct _tman_timer_man *timer_man = (struct _tman_timer_man *)tm_man;
    mb_timer_t *timer;
    struct _tman_timeout_data *tmout_data;

    tmout_data = O_ALLOC(struct _tman_timeout_data);
    tmout_data->cb = cb;
    tmout_data->data = data;
    timer = mb_tman_timeout(timer_man->tman, tmout,
			    _tman_tmo_hdlr, tmout_data);
    if(timer == NULL)
	return ERR;
    tmout_data->timer = timer;

    return (int)tmout_data;
}

static void
_tman_timer_man_remove(struct _mb_timer_man *tm_man, int tm_hdl) {
    struct _tman_timer_man *timer_man = (struct _tman_timer_man *)tm_man;
    struct _tman_timeout_data *tmout_data =
	(struct _tman_timeout_data *)tm_hdl;

    mb_tman_remove(timer_man->tman, tmout_data->timer);
    free(tmout_data);
}

static mb_timer_man_t *
_tman_timer_fact_new(void) {
    if(_tman_default_timer_man.tman == NULL)
	_tman_default_timer_man.tman = mb_tman_new();
    return (mb_timer_man_t *)&_tman_default_timer_man;
}

static void
_tman_timer_fact_free(mb_timer_man_t *timer_man) {
}

mb_tman_t *
tman_timer_man_get_tman(mb_timer_man_t *tm_man) {
    struct _tman_timer_man *timer_man = (struct _tman_timer_man *)tm_man;

    return timer_man->tman;
}


/* @} */
