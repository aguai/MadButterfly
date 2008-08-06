#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "mb_timer.h"
#include "tools.h"


#define OK 0
#define ERR -1

struct _mb_timer {
    mbsec_t sec;
    mbusec_t usec;
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
			    mbsec_t sec, mbusec_t usec,
			    mb_tmo_hdlr hdlr, void *arg) {
    mb_timer_t *timer, *visit, *last;

    timer = elmpool_elm_alloc(tman->timer_pool);
    if(timer == NULL)
	return NULL;

    timer->sec = sec;
    timer->usec = usec;
    timer->hdlr = hdlr;
    timer->arg = arg;

    last = NULL;
    for(visit = STAILQ_HEAD(tman->timers);
	visit != NULL;
	visit = STAILQ_NEXT(mb_timer_t, next, visit)) {
	if(sec < visit->sec)
	    break;
	if(sec == visit->sec && usec < visit->usec)
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
			 mbsec_t now_sec, mbusec_t now_usec,
			 mbsec_t *after_sec, mbusec_t *after_usec) {
    mb_timer_t *timer;

    timer = STAILQ_HEAD(tman->timers);
    if(timer == NULL)
	return ERR;

    if(now_sec > timer->sec ||
       (now_sec == timer->usec && now_usec >= timer->usec)) {
	*after_sec = 0;
	*after_usec = 0;
	return OK;
    }

    *after_sec = timer->sec - now_sec;
    if(now_usec > timer->usec) {
	--*after_sec;
	*after_usec = 1000000 + timer->usec - now_usec;
    } else
	*after_usec = timer->usec - now_usec;

    return OK;
}

int mb_tman_handle_timeout(mb_tman_t *tman,
			   mbsec_t now_sec, mbusec_t now_usec) {
    mb_timer_t *timer;

    while((timer = STAILQ_HEAD(tman->timers)) != NULL){
	if(now_sec < timer->sec ||
	   (now_sec == timer->sec && now_usec < timer->usec))
	    break;
	timer->hdlr(timer->sec, timer->usec,
		    now_sec, now_usec,
		    timer->arg);
	STAILQ_REMOVE(tman->timers, mb_timer_t, next, timer);
	elmpool_elm_free(tman->timer_pool, timer);
    }

    return OK;
}
