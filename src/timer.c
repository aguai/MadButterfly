#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mb_timer.h"
#include "tools.h"


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
	if(MB_TIMEVAL_LATER(&visit->tmo, tmo))
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
