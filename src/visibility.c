#include <stdio.h>
#include <stdlib.h>
#include "animate.h"

typedef struct _mb_visibility mb_visibility_t;

struct _mb_visibility {
    mb_action_t action;
    int visibility;
    coord_t *coord;
};

static void mb_visibility_start(mb_action_t *act,
				const mb_timeval_t *now,
				const mb_timeval_t *playing_time,
				redraw_man_t *rdman) {
    mb_visibility_t *visibility = (mb_visibility_t *)act;

    switch(visibility->visibility) {
    case VIS_VISIBLE:
	coord_show(visibility->coord);
	break;
    case VIS_HIDDEN:
	coord_hide(visibility->coord);
	break;
    }
    rdman_coord_changed(rdman, visibility->coord);
}

static void mb_visibility_step(mb_action_t *act,
			       const mb_timeval_t *now,
			       redraw_man_t *rdman) {
}

static void mb_visibility_stop(mb_action_t *act,
			       const mb_timeval_t *now,
			       redraw_man_t *rdman) {
}

static void mb_visibility_free(mb_action_t *act) {
    free(act);
}

mb_action_t *mb_visibility_new(int visib, coord_t *coord,
			       mb_word_t *word) {
    mb_visibility_t *visibility;

    visibility = (mb_visibility_t *)malloc(sizeof(mb_visibility_t));
    if(visibility == NULL)
	return NULL;

    visibility->visibility = visib;
    visibility->coord = coord;

    visibility->action.start = mb_visibility_start;
    visibility->action.step = mb_visibility_step;
    visibility->action.stop = mb_visibility_stop;
    visibility->action.free = mb_visibility_free;

    mb_word_add_action(word, (mb_action_t *)visibility);

    return (mb_action_t *)visibility;
}

