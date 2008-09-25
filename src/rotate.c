#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "animate.h"

static float comp_mb_timeval_ratio(const mb_timeval_t *a,
				   const mb_timeval_t *b) {
    float ratio;

    ratio = (float)MB_TIMEVAL_SEC(a) * 1000000.0 + (float)MB_TIMEVAL_USEC(a);
    ratio /= (float)MB_TIMEVAL_SEC(b) * 1000000.0 + (float)MB_TIMEVAL_USEC(b);
    return ratio;
}

/*! \brief Animation action to rotate a coordinate.
 */
struct _mb_rotate {
    mb_action_t action;

    co_aix angle1, angle2;
    coord_t *coord;

    mb_timeval_t start_time;
    const mb_timeval_t *playing_time;
};
typedef struct _mb_rotate mb_rotate_t;

static void mb_rotate_start(mb_action_t *act,
			   const mb_timeval_t *now,
			   const mb_timeval_t *playing_time,
			   redraw_man_t *rdman) {
    mb_rotate_t *rotate = (mb_rotate_t *)act;
    co_aix *matrix;
    float _sin, _cos;

    _sin = sinf(rotate->angle1);
    _cos = cosf(rotate->angle1);

    matrix = rotate->coord->matrix;
    memset(matrix, 0, sizeof(co_aix) * 6);
    matrix[0] = _cos;
    matrix[1] = -_sin;
    matrix[3] = _sin;
    matrix[4] = _cos;
    rdman_coord_changed(rdman, rotate->coord);

    MB_TIMEVAL_CP(&rotate->start_time, now);
    rotate->playing_time = playing_time;
}

static void mb_rotate_step(mb_action_t *act, const mb_timeval_t *now,
			   redraw_man_t *rdman) {
    mb_rotate_t *rotate = (mb_rotate_t *)act;
    mb_timeval_t diff;
    co_aix *matrix;
    float ratio;
    float angle;
    float _sin, _cos;

    MB_TIMEVAL_CP(&diff, now);
    MB_TIMEVAL_DIFF(&diff, &rotate->start_time);
    ratio = comp_mb_timeval_ratio(&diff, rotate->playing_time);

    angle = rotate->angle1 * (1 - ratio) + rotate->angle2 * ratio;
    _sin = sinf(angle);
    _cos = cosf(angle);

    matrix = rotate->coord->matrix;
    matrix[0] = _cos;
    matrix[1] = -_sin;
    matrix[3] = _sin;
    matrix[4] = _cos;
    rdman_coord_changed(rdman, rotate->coord);
}

static void mb_rotate_stop(mb_action_t *act, const mb_timeval_t *now,
			   redraw_man_t *rdman) {
    mb_rotate_t *rotate = (mb_rotate_t *)act;
    co_aix *matrix;
    float _sin, _cos;

    _sin = sinf(rotate->angle2);
    _cos = cosf(rotate->angle2);

    matrix = rotate->coord->matrix;
    matrix[0] = _cos;
    matrix[1] = -_sin;
    matrix[3] = _sin;
    matrix[4] = _cos;
    rdman_coord_changed(rdman, rotate->coord);
}

static void mb_rotate_free(mb_action_t *act) {
    free(act);
}

mb_action_t *mb_rotate_new(float angle1, float angle2,
			   coord_t *coord,
			   mb_word_t *word) {
    mb_rotate_t *rotate;

    rotate = (mb_rotate_t *)malloc(sizeof(mb_rotate_t));
    if(rotate == NULL)
	return NULL;

    rotate->angle1 = angle1;
    rotate->angle2 = angle2;
    rotate->coord = coord;

    rotate->action.start = mb_rotate_start;
    rotate->action.step = mb_rotate_step;
    rotate->action.stop = mb_rotate_stop;
    rotate->action.free = mb_rotate_free;

    mb_word_add_action(word, (mb_action_t *)rotate);

    return (mb_action_t *)rotate;
}
