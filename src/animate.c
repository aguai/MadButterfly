/*! \file
 * \brief Animation tools.
 *
 * \sa \ref ani
 */
/*! \page ani What is Animation?
 *
 * Animation is a program to move, resize, rotate, ..., changing
 * graphics on the output screen.
 *
 * \image html program.png
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
 *
 * A program is driven by a timer.  Once a program is started, it registers
 * with timer for periodic running.  \ref mb_tman_t is timer of
 * MadButterfly.  The update frequence is 10fps by default, now.
 *
 * \section use_progm How to Use Animation Program?
 * Following code block creates a program with 2 words.  First word is
 * started immediately after the program been started.  It is consisted
 * for 1 second.  Second word is started 1 second after the program been
 * started.  It is consisted for 2 seconds.  There are 2 action in
 * first word, they shift graphics managed by coord1 & coord2 by (50,50) and
 * (-50,50) pixels, respectly.  The shifting is performed incrementally
 * in 1 second.  Second word shifts coord1 and coord2, too. And, graphics
 * managed by coord3 are hidden at end of the word.  At end of code in the
 * block, mb_progm_start() starts the program.  3rd argument of
 * mb_progm_start() must be current wall time.
 *
 * \code
 *	progm = mb_progm_new(10, &rdman);
 *	
 *	MB_TIMEVAL_SET(&start, 0, 0);
 *	MB_TIMEVAL_SET(&playing, 1, 0);
 *	word = mb_progm_next_word(progm, &start, &playing);
 *
 *	act = mb_shift_new(50, 50, coord1, word);
 *	act = mb_shift_new(-50, 50, coord2, word);
 *
 *	MB_TIMEVAL_SET(&start, 1, 0);
 *	MB_TIMEVAL_SET(&playing, 2, 0);
 *	word = mb_progm_next_word(progm, &start, &playing);
 *
 *	act = mb_shift_new(0, 20, coord1, word);
 *	act = mb_shift_new(0, -20, coord2, word);
 *	act = mb_visibility_new(VIS_HIDDEN, coord3, word);
 *
 *	gettimeofday(&tv, NULL);
 *	MB_TIMEVAL_SET(&mbtv, tv.tv_sec, tv.tv_usec);
 *	mb_progm_start(progm, tman, &mbtv);
 * \endcode
 *
 *
 * \sa \ref animate.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mb_types.h"
#include "redraw_man.h"
#include "mb_timer.h"
#include "animate.h"


#define STEP_INTERVAL 90000
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
 *
 * first_playing is an index to one of words.  It always points to
 * the first of words that is playing or waiting for playing.
 */
struct _mb_progm {
    redraw_man_t *rdman;

    mb_timeval_t start_time;
    int first_playing;		/*!< first playing word. */
    mb_tman_t *tman;
    subject_t *complete;	/*!< notify when a program is completed. */

    int n_words;
    int max_words;
    mb_word_t words[1];
};

/*! \brief Create a program object.
 *
 * \param max_words is maximum number of words the program can hold.
 * \param rdman is a rdman that show graphics manipulated by it.
 */
mb_progm_t *mb_progm_new(int max_words, redraw_man_t *rdman) {
    mb_progm_t *progm;
    ob_factory_t *factory;
    int i;

    progm = (mb_progm_t *)malloc(sizeof(mb_progm_t) +
				 (sizeof(mb_word_t) * (max_words - 1)));
    if(progm == NULL)
	return NULL;

    progm->rdman = rdman;

    factory = rdman_get_ob_factory(rdman);
    progm->complete = subject_new(factory, progm, OBJT_PROGM);

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

void mb_word_add_action(mb_word_t *word, mb_action_t *act) {
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

/*! \brief Time stepping for a program.
 *
 * Any two actions in concurrent words never change the same attribute.
 * Start time of words in a program are specified in incremental order.
 *
 * \note Program will take a big step at last monent.  It is because
 *	mb_progm_step() running mb_word_stop() if the word being stop
 *	between now and next_tmo.  It is not obviously if time stepping
 *	small.
 */
static void mb_progm_step(const mb_timeval_t *tmo,
			  const mb_timeval_t *now,
			  void *arg) {
    mb_progm_t *progm = (mb_progm_t *)arg;
    ob_factory_t *factory;
    mb_progm_complete_t comp_evt;
    mb_timeval_t next_tmo;
    mb_word_t *word;
    mb_timer_t *timer;
    int i;

    MB_TIMEVAL_SET(&next_tmo, 0, STEP_INTERVAL);
    MB_TIMEVAL_ADD(&next_tmo, now);

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
    } else {
	factory = rdman_get_ob_factory(progm->rdman);
	comp_evt.event.type = EVT_PROGM_COMPLETE;
	comp_evt.event.tgt = comp_evt.event.cur_tgt = progm->complete;
	comp_evt.progm = progm;
	subject_notify(factory, progm->complete, &comp_evt.event);
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

/*! \brief Return event subject for completion of a program.
 */
subject_t *mb_progm_get_complete(mb_progm_t *progm) {
    return progm->complete;
}

#ifdef UNITTEST

#include <CUnit/Basic.h>

typedef struct _mb_dummy mb_dummy_t;

struct _mb_dummy {
    mb_action_t action;
    int id;
    int *logidx;
    int *logs;
};


static void mb_dummy_start(mb_action_t *act,
			   const mb_timeval_t *now,
			   const mb_timeval_t *playing_time,
			   redraw_man_t *rdman) {
    mb_dummy_t *dummy = (mb_dummy_t *)act;

    dummy->logs[(*dummy->logidx)++] = dummy->id;
}

static void mb_dummy_step(mb_action_t *act,
			  const mb_timeval_t *now,
			  redraw_man_t *rdman) {
    mb_dummy_t *dummy = (mb_dummy_t *)act;

    dummy->logs[(*dummy->logidx)++] = dummy->id;
}

static void mb_dummy_stop(mb_action_t *act,
			  const mb_timeval_t *now,
			  redraw_man_t *rdman) {
    mb_dummy_t *dummy = (mb_dummy_t *)act;

    dummy->logs[(*dummy->logidx)++] = dummy->id;
}

static void mb_dummy_free(mb_action_t *act) {
    free(act);
}

mb_action_t *mb_dummy_new(int id, int *logidx, int *logs, mb_word_t *word) {
    mb_dummy_t *dummy;

    dummy = (mb_dummy_t *)malloc(sizeof(mb_dummy_t));
    if(dummy == NULL)
	return NULL;

    dummy->id = id;
    dummy->logidx = logidx;
    dummy->logs = logs;

    dummy->action.start = mb_dummy_start;
    dummy->action.step = mb_dummy_step;
    dummy->action.stop = mb_dummy_stop;
    dummy->action.free = mb_dummy_free;

    mb_word_add_action(word, (mb_action_t *)dummy);

    return (mb_action_t *)dummy;
}

void test_animate_words(void) {
    mb_progm_t *progm;
    mb_word_t *word;
    mb_action_t *acts[4];
    mb_tman_t *tman;
    mb_timeval_t tv1, tv2, now, tmo_after;
    int logcnt = 0;
    int logs[256];
    int r;

    tman = mb_tman_new();
    CU_ASSERT(tman != NULL);

    progm = mb_progm_new(3, NULL);
    CU_ASSERT(progm != NULL);

    MB_TIMEVAL_SET(&tv1, 1, 0);
    MB_TIMEVAL_SET(&tv2, 0, STEP_INTERVAL * 3);
    word = mb_progm_next_word(progm, &tv1, &tv2);
    CU_ASSERT(word != NULL);
    acts[0] = mb_dummy_new(0, &logcnt, logs, word);
    CU_ASSERT(acts[0] != NULL);

    MB_TIMEVAL_SET(&tv1, 1, STEP_INTERVAL * 4 / 3);
    MB_TIMEVAL_SET(&tv2, 0, STEP_INTERVAL / 3);
    word = mb_progm_next_word(progm, &tv1, &tv2);
    CU_ASSERT(word != NULL);
    acts[1] = mb_dummy_new(1, &logcnt, logs, word);
    CU_ASSERT(acts[1] != NULL);

    MB_TIMEVAL_SET(&tv1, 1, STEP_INTERVAL * 2);
    MB_TIMEVAL_SET(&tv2, 0, STEP_INTERVAL * 3);
    word = mb_progm_next_word(progm, &tv1, &tv2);
    CU_ASSERT(word != NULL);
    acts[2] = mb_dummy_new(2, &logcnt, logs, word);
    CU_ASSERT(acts[2] != NULL);

    MB_TIMEVAL_SET(&now, 0, 0);
    mb_progm_start(progm, tman, &now);

    r = mb_tman_next_timeout(tman, &now, &tmo_after);
    CU_ASSERT(r == 0);
    CU_ASSERT(MB_TIMEVAL_SEC(&tmo_after) == 1 &&
	      MB_TIMEVAL_USEC(&tmo_after) == 0);

    /* 1.0s */
    MB_TIMEVAL_ADD(&now, &tmo_after);
    mb_tman_handle_timeout(tman, &now);
    CU_ASSERT(logcnt == 1);

    r = mb_tman_next_timeout(tman, &now, &tmo_after);
    CU_ASSERT(r == 0);
    CU_ASSERT(MB_TIMEVAL_SEC(&tmo_after) == 0 &&
	      MB_TIMEVAL_USEC(&tmo_after) == STEP_INTERVAL);
    
    /* 1.1s */
    MB_TIMEVAL_ADD(&now, &tmo_after);
    mb_tman_handle_timeout(tman, &now);
    CU_ASSERT(logcnt == 4);

    r = mb_tman_next_timeout(tman, &now, &tmo_after);
    CU_ASSERT(r == 0);
    CU_ASSERT(MB_TIMEVAL_SEC(&tmo_after) == 0 &&
	      MB_TIMEVAL_USEC(&tmo_after) == STEP_INTERVAL);
    
    /* 1.2s */
    MB_TIMEVAL_ADD(&now, &tmo_after);
    mb_tman_handle_timeout(tman, &now);
    CU_ASSERT(logcnt == 6);

    r = mb_tman_next_timeout(tman, &now, &tmo_after);
    CU_ASSERT(r == 0);
    CU_ASSERT(MB_TIMEVAL_SEC(&tmo_after) == 0 &&
	      MB_TIMEVAL_USEC(&tmo_after) == STEP_INTERVAL);
    
    /* 1.3s */
    MB_TIMEVAL_ADD(&now, &tmo_after);
    mb_tman_handle_timeout(tman, &now);
    CU_ASSERT(logcnt == 8);

    r = mb_tman_next_timeout(tman, &now, &tmo_after);
    CU_ASSERT(r == 0);
    CU_ASSERT(MB_TIMEVAL_SEC(&tmo_after) == 0 &&
	      MB_TIMEVAL_USEC(&tmo_after) == STEP_INTERVAL);
    
    /* 1.4s */
    MB_TIMEVAL_ADD(&now, &tmo_after);
    mb_tman_handle_timeout(tman, &now);
    CU_ASSERT(logcnt == 9);

    r = mb_tman_next_timeout(tman, &now, &tmo_after);
    CU_ASSERT(r == 0);
    CU_ASSERT(MB_TIMEVAL_SEC(&tmo_after) == 0 &&
	      MB_TIMEVAL_USEC(&tmo_after) == STEP_INTERVAL);
    
    /* 1.5s */
    MB_TIMEVAL_ADD(&now, &tmo_after);
    mb_tman_handle_timeout(tman, &now);
    CU_ASSERT(logcnt == 10);

    r = mb_tman_next_timeout(tman, &now, &tmo_after);
    CU_ASSERT(r == -1);

    mb_progm_free(progm);
    mb_tman_free(tman);
}

CU_pSuite get_animate_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_animate", NULL, NULL);
    if(suite == NULL)
	return NULL;

    CU_ADD_TEST(suite, test_animate_words);
    
    return suite;
}

#endif /* UNITTEST */
