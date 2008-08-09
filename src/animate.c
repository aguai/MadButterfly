/*! \brief Animation tools.
 *
 * XXX: Program is a sequence of actions duration a perior.
 * Actions are grouped into words.  A program defines
 * the order and time of playing of words.  A word
 * defines how long to perform a set of actions.  Actions
 * in a word are performed concurrently.
 *
 * Animation shapes are updated periodically.  Every action
 * are working with start, step, and stop 3 calls.  start is
 * called when start time the word, contain it, due.  step is
 * called periodically in duration of playing time start at
 * 'start time'.  When the clock run out the playing time of
 * a word, it call stop of actions in the word.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mb_types.h"
#include "redraw_man.h"
#include "mb_timer.h"
#include "animate.h"


#define STEP_INTERVAL 100000
#define ASSERT(x)

/*! \brief A word is a set of concurrent actions in a program.
 */
struct _mb_word {
    mb_timeval_t start_time;	/*!< time to start the word */
    mb_timeval_t playing_time;	/*!< time duration of playing */
    mb_timeval_t abs_start, abs_stop;
    STAILQ(mb_action_t) actions;
};

/*! \brief A program describe a series of actions to animate shapes.
 */
struct _mb_progm {
    redraw_man_t *rdman;

    mb_timeval_t start_time;
    int first_playing;		/*!< first playing word. */
    mb_tman_t *tman;

    int n_words;
    int max_words;
    mb_word_t words[1];
};

/*! \brief Basic class of nnimation actions.
 */
struct _mb_action {
    int act_type;
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

mb_progm_t *mb_progm_new(int max_words, redraw_man_t *rdman) {
    mb_progm_t *progm;
    int i;

    progm = (mb_progm_t *)malloc(sizeof(mb_progm_t) +
				 (sizeof(mb_word_t) * (max_words - 1)));
    if(progm == NULL)
	return NULL;

    progm->rdman = rdman;
    progm->n_words = 0;
    progm->max_words = max_words;
    for(i = 0; i < max_words; i++)
	STAILQ_INIT(progm->words[i].actions);
    return progm;
}

void mb_progm_free(mb_progm_t *progm) {
    int n_words;
    mb_word_t *word;
    mb_action_t *cur_act;
    int i;

    n_words = progm->n_words;
    for(i = 0; i < n_words; i++) {
	word = progm->words + i;
	for(cur_act = STAILQ_HEAD(word->actions);
	    cur_act != NULL;
	    cur_act = STAILQ_HEAD(word->actions)) {
	    STAILQ_REMOVE(word->actions, mb_action_t, next, cur_act);
	    cur_act->free(cur_act);
	}
	free(word);
    }
    free(progm);
}

/*! \brief Add a new word into a program.
 *
 * The start time of new word should bigger or equal to last one.
 * The time should be specified in incremental order.
 */
mb_word_t *mb_progm_next_word(mb_progm_t *progm,
			      const mb_timeval_t *start,
			      const mb_timeval_t *playing) {
    mb_word_t *word;
    if(progm->n_words >= progm->max_words)
	return NULL;
    if(progm->n_words &&
       MB_TIMEVAL_LATER(&progm->words[progm->n_words - 1].start_time, start))
	return NULL;
    word = progm->words + progm->n_words++;
    MB_TIMEVAL_CP(&word->start_time, start);
    MB_TIMEVAL_CP(&word->playing_time, playing);
    return word;
}

static void mb_word_add_action(mb_word_t *word, mb_action_t *act) {
    STAILQ_INS_TAIL(word->actions, mb_action_t, next, act);
}

static void mb_word_start(mb_word_t *word, const mb_timeval_t *tmo,
			  const mb_timeval_t *now, redraw_man_t *rdman) {
    mb_action_t *act;

    for(act = STAILQ_HEAD(word->actions);
	act != NULL;
	act = STAILQ_NEXT(mb_action_t, next, act)) {
	act->start(act, tmo, &word->playing_time, rdman);
    }
}

static void mb_word_step(mb_word_t *word, const mb_timeval_t *tmo,
			 const mb_timeval_t *now, redraw_man_t *rdman) {
    mb_action_t *act;

    for(act = STAILQ_HEAD(word->actions);
	act != NULL;
	act = STAILQ_NEXT(mb_action_t, next, act)) {
	act->step(act, tmo, rdman);
    }
}

static void mb_word_stop(mb_word_t *word, const mb_timeval_t *tmo,
			 const mb_timeval_t *now, redraw_man_t *rdman) {
    mb_action_t *act;

    for(act = STAILQ_HEAD(word->actions);
	act != NULL;
	act = STAILQ_NEXT(mb_action_t, next, act)) {
	act->stop(act, tmo, rdman);
    }
}

/*
 * Any two actions in concurrent words never change the same attribute.
 * Start time of words in a program are specified in incremental order.
 */
static void mb_progm_step(const mb_timeval_t *tmo,
			     const mb_timeval_t *now,
			     void *arg) {
    mb_progm_t *progm = (mb_progm_t *)arg;
    mb_timeval_t next_tmo;
    mb_word_t *word;
    mb_timer_t *timer;
    int i;

    MB_TIMEVAL_SET(&next_tmo, 0, STEP_INTERVAL);
    MB_TIMEVAL_ADD(&next_tmo, tmo);

    /* _step() or _stop() words that in playing. */
    i = progm->first_playing;
    for(word = progm->words + i;
	i < progm->n_words && MB_TIMEVAL_LATER(tmo, &word->abs_start);
	word = progm->words + ++i) {
	if(MB_TIMEVAL_LATER(tmo, &word->abs_stop))
	    continue;
	if(MB_TIMEVAL_LATER(&next_tmo, &word->abs_stop))
	    mb_word_stop(word, tmo, now, progm->rdman);
	else
	    mb_word_step(word, tmo, now, progm->rdman);
    }

    /* Start words that their abs_start is in duration
     * from now to timeout for next update.
     */
    for(word = progm->words + i;
	i < progm->n_words && MB_TIMEVAL_LATER(&next_tmo, &word->abs_start);
	word = progm->words + ++i) {
	mb_word_start(word, tmo, now, progm->rdman);
	if(MB_TIMEVAL_LATER(&next_tmo, &word->abs_stop))
	    mb_word_stop(word, tmo, now, progm->rdman);
    }

    /* Find a new first_playing if any consequence words, following current
     * first_playing word, are stoped.
     */
    i = progm->first_playing;
    for(word = progm->words + i;
	i < progm->n_words && MB_TIMEVAL_LATER(&next_tmo, &word->abs_stop);
	word = progm->words + ++i) {
	progm->first_playing++;
    }

    /* Setup timeout for next update. */
    if(progm->first_playing < progm->n_words) {
	word = progm->words + progm->first_playing;
	if(MB_TIMEVAL_LATER(&word->abs_start, &next_tmo))
	    MB_TIMEVAL_CP(&next_tmo, &word->abs_start);
	timer = mb_tman_timeout(progm->tman, &next_tmo,
				mb_progm_step, progm);	
    }
}

void mb_progm_start(mb_progm_t *progm, mb_tman_t *tman,
		    mb_timeval_t *now) {
    mb_timer_t *timer;
    mb_word_t *word;
    int i;

    if(progm->n_words == 0)
	return;

    progm->tman = tman;
    MB_TIMEVAL_CP(&progm->start_time, now);
    progm->first_playing = 0;

    for(i = 0; i < progm->n_words; i++) {
	word = progm->words + i;
	MB_TIMEVAL_CP(&word->abs_start, &word->start_time);
	MB_TIMEVAL_ADD(&word->abs_start, now);
	MB_TIMEVAL_CP(&word->abs_stop, &word->abs_start);
	MB_TIMEVAL_ADD(&word->abs_stop, &word->playing_time);
    }

    if(MB_TIMEVAL_EQ(&progm->words[0].abs_start, now)) {
	mb_progm_step(now, now, progm);
	return;
    }
    
    timer = mb_tman_timeout(tman, &progm->words[0].abs_start,
			    mb_progm_step, progm);
    ASSERT(timer != NULL);
}

void mb_progm_abort(mb_progm_t *progm, mb_tman_t *tman) {
}

typedef struct _mb_shift mb_shift_t;
/*! \brief Animation action for shift a coordination. */
struct _mb_shift {
    mb_action_t action;

    co_aix x, y;
    coord_t *coord;

    mb_timeval_t start_time;
    co_aix saved_matrix[6];
    const mb_timeval_t *playing_time;
};

static float comp_mb_timeval_ratio(mb_timeval_t *a, const mb_timeval_t *b) {
    float ratio;

    ratio = (float)MB_TIMEVAL_SEC(a) * 1000000.0 + (float)MB_TIMEVAL_USEC(a);
    ratio /= (float)MB_TIMEVAL_SEC(b) * 1000000.0 + (float)MB_TIMEVAL_USEC(b);
    return ratio;
}

static void mb_shift_start(mb_action_t *act,
			   const mb_timeval_t *now,
			   const mb_timeval_t *playing_time,
			   redraw_man_t *rdman) {
    mb_shift_t *shift = (mb_shift_t *)act;
    coord_t *coord;

    MB_TIMEVAL_CP(&shift->start_time, now);
    coord = shift->coord;
    memcpy(&shift->saved_matrix, coord->matrix, sizeof(co_aix[6]));
    shift->playing_time = playing_time;
}

static void mb_shift_step(mb_action_t *act, const mb_timeval_t *now,
			  redraw_man_t *rdman) {
    mb_shift_t *shift = (mb_shift_t *)act;
    mb_timeval_t diff;
    coord_t *coord;
    float ratio;

    
    MB_TIMEVAL_CP(&diff, now);
    MB_TIMEVAL_DIFF(&diff, &shift->start_time);
    ratio = comp_mb_timeval_ratio(&diff, shift->playing_time);

    coord = shift->coord;
    coord->matrix[2] = shift->saved_matrix[2] + shift->x * ratio;
    coord->matrix[5] = shift->saved_matrix[5] + shift->y * ratio;

    rdman_coord_changed(rdman, coord);
}

static void mb_shift_stop(mb_action_t *act, const mb_timeval_t *now,
			  redraw_man_t *rdman) {
    mb_shift_t *shift = (mb_shift_t *)act;
    coord_t *coord;

    coord = shift->coord;
    coord->matrix[2] = shift->saved_matrix[2] + shift->x;
    coord->matrix[5] = shift->saved_matrix[5] + shift->y;

    rdman_coord_changed(rdman, coord);
}


static void mb_shift_free(mb_action_t *act) {
    free(act);
}

mb_action_t *mb_shift_new(co_aix x, co_aix y, coord_t *coord,
			  mb_word_t *word) {
    mb_shift_t *shift;

    shift = (mb_shift_t *)malloc(sizeof(mb_shift_t));
    if(shift == NULL)
	return (mb_action_t *)shift;

    shift->x = x;
    shift->y = y;
    shift->coord = coord;

    shift->action.start = mb_shift_start;
    shift->action.step = mb_shift_step;
    shift->action.stop = mb_shift_stop;
    shift->action.free = mb_shift_free;

    mb_word_add_action(word, (mb_action_t *)shift);

    return (mb_action_t *)shift;
}
