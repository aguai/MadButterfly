#ifndef __ANIMATE_H_
#define __ANIMATE_H_

typedef struct _mb_progm mb_progm_t;
typedef struct _mb_word mb_word_t;
typedef struct _mb_action mb_action_t;
typedef struct _mb_progm_state mb_progm_state_t;

extern mb_progm_t *mb_progm_new(int max_words, redraw_man_t *rdman);
extern void mb_progm_free(mb_progm_t *progm);
extern mb_word_t *mb_progm_next_word(mb_progm_t *progm,
				     const mb_timeval_t *start,
				     const mb_timeval_t *playing);
extern void mb_word_add_action(mb_word_t *word, mb_action_t *act);
extern mb_action_t *mb_shift_new(co_aix x, co_aix y);

#endif /* __ANIMATE_H_ */
