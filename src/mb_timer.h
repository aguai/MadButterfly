#ifndef __MB_TIMER_H_
#define __MB_TIMER_H_

typedef uint32_t mbsec_t;
typedef uint32_t mbusec_t;
typedef struct _mb_timer mb_timer_t;
typedef struct _mb_tman mb_tman_t;

typedef void (*mb_tmo_hdlr)(mbsec_t sec, mbusec_t usec,
			    mbsec_t now_sec, mbusec_t now_usec,
			    void *arg);

extern mb_tman_t *mb_tman_new(void);
extern void mb_tman_free(mb_tman_t *tman);
extern mb_timer_t *mb_tman_timeout(mb_tman_t *tman,
				   mbsec_t sec, mbusec_t usec,
				   mb_tmo_hdlr hdlr, void *arg);
extern int mb_tman_remove(mb_tman_t *tman, mb_timer_t *timer);
extern int mb_tman_next_timeout(mb_tman_t *tman,
				mbsec_t now_sec, mbusec_t now_usec,
				mbsec_t *after_sec, mbusec_t *after_usec);
extern int mb_tman_handle_timeout(mb_tman_t *tman,
				  mbsec_t now_sec, mbusec_t now_usec);



#endif /* __MB_TIMER_H_ */
