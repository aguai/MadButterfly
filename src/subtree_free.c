#include "mb_redraw_man.h"
#include "mb_animate.h"

struct _subtree_free {
    mb_action_t action;

    coord_t *coord;
};

typedef struct _subtree_free subtree_free_t;

void mb_subtree_free_start(mb_action_t *action,
			   const mb_timeval_t *now,
			   const mb_timeval_t *playing_time,
			   redraw_man_t *rdman) {
    subtree_free_t *sfree = (subtree_free_t *)action;

    rdman_coord_subtree_free(rdman, sfree->coord);
}

void mb_subtree_free_step(mb_action_t *act, const mb_timeval_t *now,
			  redraw_man_t *rdman) {
}

void mb_subtree_free_stop(mb_action_t *act, const mb_timeval_t *now,
			  redraw_man_t *rdman) {
}

void mb_subtree_free_free(mb_action_t *act) {
    free(act);
}

mb_action_t *mb_subtree_free_new(coord_t *coord,
				 mb_word_t *word) {
    subtree_free_t *sfree;

    sfree = (subtree_free_t *)malloc(sizeof(subtree_free_t));
    if(sfree == NULL)
	return NULL;

    sfree->action.start = mb_subtree_free_start;
    sfree->action.step = mb_subtree_free_step;
    sfree->action.stop = mb_subtree_free_stop;
    sfree->action.free = mb_subtree_free_free;
    sfree->coord = coord;

    mb_word_add_action(word, (mb_action_t *)sfree);

    return (mb_action_t *)sfree;
}
