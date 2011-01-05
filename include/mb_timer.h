// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __MB_TIMER_H_
#define __MB_TIMER_H_

#include <sys/time.h>
#include <stdint.h>

typedef uint32_t mbsec_t;
typedef uint32_t mbusec_t;
typedef struct _mb_timer mb_timer_t;
typedef struct _mb_tman mb_tman_t;
typedef struct timeval mb_timeval_t;


typedef void (*mb_tmo_hdlr)(const mb_timeval_t *tmo,
			    const mb_timeval_t *now,
			    void *arg);

extern mb_tman_t *mb_tman_new(void);
extern void mb_tman_free(mb_tman_t *tman);
extern mb_timer_t *mb_tman_timeout(mb_tman_t *tman,
				   const mb_timeval_t *tmo,
				   mb_tmo_hdlr hdlr, void *arg);
extern int mb_tman_remove(mb_tman_t *tman, mb_timer_t *timer);
extern int mb_tman_next_timeout(mb_tman_t *tman,
				const mb_timeval_t *now,
				mb_timeval_t *tmo_after);
extern int mb_tman_handle_timeout(mb_tman_t *tman, mb_timeval_t *now);

#define MB_TIMEVAL_SET(_tv, _sec, _usec)	\
    do {					\
	(_tv)->tv_sec = _sec;			\
	(_tv)->tv_usec = _usec;			\
    } while(0)
#define MB_TIMEVAL_CP(_tv1, _tv2)		\
    do {					\
	(_tv1)->tv_sec = (_tv2)->tv_sec;	\
	(_tv1)->tv_usec = (_tv2)->tv_usec;	\
    } while(0)
#define MB_TIMEVAL_SEC(_tv) ((_tv)->tv_sec)
#define MB_TIMEVAL_USEC(_tv) ((_tv)->tv_usec)
#define MB_TIMEVAL_LATER(a, b)			\
    ((a)->tv_sec > (b)->tv_sec ||		\
     ((a)->tv_sec == (b)->tv_sec &&		\
      (a)->tv_usec > (b)->tv_usec))
#define MB_TIMEVAL_LATER_INC(a, b)		\
    ((a)->tv_sec > (b)->tv_sec ||		\
     ((a)->tv_sec == (b)->tv_sec &&		\
      (a)->tv_usec >= (b)->tv_usec))
#define MB_TIMEVAL_EQ(a, b)			\
    ((a)->tv_sec == (b)->tv_sec &&		\
     (a)->tv_usec == (b)->tv_usec)
#define MB_TIMEVAL_DIFF(a, b)			\
    do {					\
	(a)->tv_sec -= (b)->tv_sec;		\
	if((a)->tv_usec < (b)->tv_usec) {	\
	    (a)->tv_sec--;			\
	    (a)->tv_usec += 1000000;		\
	}					\
	(a)->tv_usec -= (b)->tv_usec;		\
    } while(0)
#define MB_TIMEVAL_ADD(a, b)			\
    do {					\
	(a)->tv_sec += (b)->tv_sec;		\
	(a)->tv_usec += (b)->tv_usec;		\
	if((a)->tv_usec >= 1000000) {		\
	    (a)->tv_sec++;			\
	    (a)->tv_usec -= 1000000;		\
	}					\
    } while(0)
#define MB_TIMEVAL_DIV(a, b)			\
    (((a)->tv_sec * 1000000.0 + (a)->tv_usec) /	\
     ((b)->tv_sec * 1000000.0 + (b)->tv_usec))


extern void get_now(mb_timeval_t *tmo);


#endif /* __MB_TIMER_H_ */
