// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __ANIMATE_H_
#define __ANIMATE_H_

#include "mb_types.h"
#include "mb_timer.h"
#include "mb_paint.h"

/*! \page def_action How to Define An Action?
 *
 * A action must implement following 4 functions.
 * \li start,
 * \li step,
 * \li stop,
 * \li free,
 * \li *_new(), and
 * \li add *_new() to \ref animate.h .
 *
 * *_new() must invokes mb_word_add_action() to add new object
 * as one of actions in the word specified as an argument of it.
 * It also means *_new() must have an argument with type of
 * (mb_word_t *).
 */

/*! \defgroup anim Animation
 * \brief Animation is a set of functions to make graph moving.
 * @{
 */
typedef struct _mb_progm mb_progm_t;
typedef struct _mb_word mb_word_t;
typedef struct _mb_action mb_action_t;

struct _mb_progm_complete {
    event_t event;
    mb_progm_t *progm;
};
typedef struct _mb_progm_complete mb_progm_complete_t;

extern mb_progm_t *mb_progm_new(int max_words, redraw_man_t *rdman);
extern void mb_progm_free(mb_progm_t *progm);
extern mb_word_t *mb_progm_next_word(mb_progm_t *progm,
				     const mb_timeval_t *start,
				     const mb_timeval_t *playing);
extern void mb_progm_start(mb_progm_t *progm, mb_tman_t *tman,
			   mb_timeval_t *now);
extern void mb_progm_abort(mb_progm_t *progm);
extern subject_t *mb_progm_get_complete(mb_progm_t *progm);
extern void mb_progm_free_completed(mb_progm_t *progm);

/*! \defgroup ani_actions Animation Actions
 * @{
 */
extern mb_action_t *mb_shift_new(co_aix x, co_aix y, coord_t *coord,
				 mb_word_t *word);
extern mb_action_t *mb_chgcolor_new(co_comp_t r, co_comp_t g,
				    co_comp_t b, co_comp_t a,
				    paint_t *paint, mb_word_t *word);
extern mb_action_t *mb_rotate_new(float angle1, float angle2,
				  coord_t *coord, mb_word_t *word);
extern mb_action_t *mb_subtree_free_new(coord_t *coord,
					mb_word_t *word);

enum { VIS_VISIBLE, VIS_HIDDEN };
extern mb_action_t *mb_visibility_new(int visib, coord_t *coord,
				      mb_word_t *word);
/* @} */

/*! \defgroup act_support Action Supports.
 * @{
 */
/*! \brief Basic class of animation actions.
 *
 * \sa \ref def_action
 */
struct _mb_action {
    void (*start)(mb_action_t *act,
		  const mb_timeval_t *now,
		  const mb_timeval_t *playing_time,
		  redraw_man_t *rdman);
    void (*step)(mb_action_t *act, const mb_timeval_t *now,
		 redraw_man_t *rdman);
    void (*stop)(mb_action_t *act, const mb_timeval_t *now,
		 redraw_man_t *rdman);
    void (*free)(mb_action_t *act);
    mb_action_t *next;
};

extern void mb_word_add_action(mb_word_t *word, mb_action_t *act);
/* @} */

/* @} */

#endif /* __ANIMATE_H_ */
