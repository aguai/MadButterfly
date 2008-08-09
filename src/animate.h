#ifndef __ANIMATE_H_
#define __ANIMATE_H_

#include "mb_types.h"
#include "mb_timer.h"
#include "paint.h"

typedef struct _mb_progm mb_progm_t;
typedef struct _mb_word mb_word_t;
typedef struct _mb_action mb_action_t;
typedef struct _mb_progm_state mb_progm_state_t;

extern mb_progm_t *mb_progm_new(int max_words, redraw_man_t *rdman);
extern void mb_progm_free(mb_progm_t *progm);
extern mb_word_t *mb_progm_next_word(mb_progm_t *progm,
				     const mb_timeval_t *start,
				     const mb_timeval_t *playing);
extern void mb_progm_start(mb_progm_t *progm, mb_tman_t *tman,
			   mb_timeval_t *now);
extern void mb_progm_abort(mb_progm_t *progm, mb_tman_t *tman);
extern mb_action_t *mb_shift_new(co_aix x, co_aix y, coord_t *coord,
				 mb_word_t *word);
extern mb_action_t *mb_chgcolor_new(co_comp_t r, co_comp_t g,
				    co_comp_t b, co_comp_t a,
				    paint_t *paint, mb_word_t *word);


#endif /* __ANIMATE_H_ */
