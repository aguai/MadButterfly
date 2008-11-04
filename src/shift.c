#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mb/animate.h"

static float comp_mb_timeval_ratio(const mb_timeval_t *a,
				   const mb_timeval_t *b) {
    float ratio;

    ratio = (float)MB_TIMEVAL_SEC(a) * 1000000.0 + (float)MB_TIMEVAL_USEC(a);
    ratio /= (float)MB_TIMEVAL_SEC(b) * 1000000.0 + (float)MB_TIMEVAL_USEC(b);
    return ratio;
}

typedef struct _mb_shift mb_shift_t;
/*! \brief Animation action for relative shift a coordination. */
struct _mb_shift {
    mb_action_t action;

    co_aix x, y;
    coord_t *coord;

    mb_timeval_t start_time;
    co_aix saved_matrix[6];
    const mb_timeval_t *playing_time;
};

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

    /*! \note mb_shift_new() will add itself to the specified word. */
    mb_word_add_action(word, (mb_action_t *)shift);

    return (mb_action_t *)shift;
}
