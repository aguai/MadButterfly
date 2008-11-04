#include <stdio.h>
#include <stdlib.h>
#include "mb/animate.h"
#include "mb/paint.h"

static float comp_mb_timeval_ratio(const mb_timeval_t *a,
				   const mb_timeval_t *b) {
    float ratio;

    ratio = (float)MB_TIMEVAL_SEC(a) * 1000000.0 + (float)MB_TIMEVAL_USEC(a);
    ratio /= (float)MB_TIMEVAL_SEC(b) * 1000000.0 + (float)MB_TIMEVAL_USEC(b);
    return ratio;
}

typedef struct _mb_chgcolor mb_chgcolor_t;

struct _mb_chgcolor {
    mb_action_t action;

    co_comp_t r, g, b, a;
    paint_t *paint;

    mb_timeval_t start_time;
    const mb_timeval_t *playing_time;
    co_comp_t s_r, s_g, s_b, s_a; /*!< saved RGBA values. */
};

static void mb_chgcolor_start(mb_action_t *act,
			      const mb_timeval_t *now,
			      const mb_timeval_t *playing_time,
			      redraw_man_t *rdman) {
    mb_chgcolor_t *chg = (mb_chgcolor_t *)act;

    MB_TIMEVAL_CP(&chg->start_time, now);
    chg->playing_time = playing_time; /* playing_time is in word,
				       * it live time is as long as
				       * actions. */
    paint_color_get(chg->paint,
		    &chg->s_r, &chg->s_g,
		    &chg->s_b, &chg->s_a);
}

static void mb_chgcolor_step(mb_action_t *act,
			     const mb_timeval_t *now,
			     redraw_man_t *rdman) {
    mb_chgcolor_t *chg = (mb_chgcolor_t *)act;
    mb_timeval_t diff;
    co_comp_t r, g, b, a;
    float ratio, comp;

    MB_TIMEVAL_CP(&diff, now);
    MB_TIMEVAL_DIFF(&diff, &chg->start_time);
    ratio = comp_mb_timeval_ratio(&diff, chg->playing_time);
    comp = 1 - ratio;

    r = chg->s_r * comp + ratio * chg->r;
    g = chg->s_g * comp + ratio * chg->g;
    b = chg->s_b * comp + ratio * chg->b;
    a = chg->s_a * comp + ratio * chg->a;
    paint_color_set(chg->paint, r, g, b, a);

    rdman_paint_changed(rdman, chg->paint);
}

static void mb_chgcolor_stop(mb_action_t *act,
			     const mb_timeval_t *now,
			     redraw_man_t *rdman) {
    mb_chgcolor_t *chg = (mb_chgcolor_t *)act;

    paint_color_set(chg->paint, chg->r, chg->g, chg->b, chg->a);

    rdman_paint_changed(rdman, chg->paint);
}

static void mb_chgcolor_free(mb_action_t *act) {
    free(act);
}

mb_action_t *mb_chgcolor_new(co_comp_t r, co_comp_t g,
			     co_comp_t b, co_comp_t a,
			     paint_t *paint, mb_word_t *word) {
    mb_chgcolor_t *chg;

    chg = (mb_chgcolor_t *)malloc(sizeof(mb_chgcolor_t));
    if(chg == NULL)
	return NULL;

    chg->r = r;
    chg->g = g;
    chg->b = b;
    chg->a = a;

    chg->paint = paint;

    chg->action.start = mb_chgcolor_start;
    chg->action.step = mb_chgcolor_step;
    chg->action.stop = mb_chgcolor_stop;
    chg->action.free = mb_chgcolor_free;

    mb_word_add_action(word, (mb_action_t *)chg);

    return (mb_action_t *)chg;
}


