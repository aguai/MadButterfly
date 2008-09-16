#include <sys/time.h>
#include <mb/mb.h>
#include "svgs.h"

enum { MUD, ROC, BRI, BSH };

static char map[12][16] = {
    { MUD, MUD, MUD, MUD, MUD, MUD, MUD, MUD,
      MUD, MUD, MUD, MUD, MUD, MUD, MUD, MUD},
    { MUD, ROC, ROC, ROC, MUD, BSH, BSH, ROC,
      BSH, ROC, MUD, BSH, BSH, ROC, ROC, MUD},
    { MUD, MUD, BRI, MUD, MUD, MUD, MUD, MUD,
      MUD, MUD, MUD, BRI, MUD, MUD, BSH, MUD},
    { BRI, MUD, MUD, MUD, MUD, MUD, BRI, MUD,
      BRI, MUD, MUD, MUD, MUD, MUD, MUD, MUD},
    { MUD, MUD, BRI, MUD, BRI, BSH, BRI, BRI,
      BRI, BRI, BSH, ROC, ROC, MUD, BRI, MUD},
    { MUD, BRI, BRI, MUD, BRI, MUD, BRI, MUD,
      ROC, MUD, MUD, MUD, MUD, MUD, MUD, MUD},
    { MUD, MUD, MUD, MUD, MUD, MUD, MUD, MUD,
      MUD, MUD, MUD, BRI, BRI, BRI, BRI, MUD},
    { MUD, BRI, MUD, BRI, BRI, MUD, BRI, BRI,
      BSH, BRI, MUD, MUD, MUD, MUD, MUD, MUD},
    { MUD, BRI, MUD, MUD, MUD, MUD, MUD, MUD,
      MUD, MUD, MUD, BRI, BRI, MUD, BRI, BRI},
    { MUD, BRI, MUD, BRI, BRI, MUD, BRI, BRI,
      BRI, BRI, MUD, BRI, MUD, MUD, MUD, MUD},
    { MUD, BSH, MUD, BRI, MUD, MUD, BRI, MUD,
      MUD, BRI, MUD, MUD, MUD, BRI, BRI, MUD},
    { MUD, MUD, MUD, MUD, MUD, MUD, BRI, MUD,
      MUD, BRI, MUD, BRI, MUD, MUD, MUD, MUD}
};

typedef struct _tank_rt tank_rt_t;

struct _tank_rt {
    tank1_t *tank1;
    tank2_t *tank2;
    int n_enemy;
    tank_en_t *tank_enemies[10];
    void *map[12][16];
    X_MB_runtime_t *mb_rt;

    mb_progm_t *tank1_progm;

    observer_t *kb_observer;
};

#define CHANGE_POS(g, x, y) do {			\
	(g)->root_coord->matrix[0] = 1.0;		\
	(g)->root_coord->matrix[2] = x;			\
	(g)->root_coord->matrix[4] = 1.0;		\
	(g)->root_coord->matrix[5] = y;			\
	rdman_coord_changed(rdman, (g)->root_coord);	\
    } while(0)

static void free_progm_handler(event_t *event, void *arg) {
    tank_rt_t *tank_rt = (tank_rt_t *)arg;

    mb_progm_free(tank_rt->tank1_progm);
    tank_rt->tank1_progm = NULL;
}

static void keyboard_handler(event_t *event, void *arg) {
    X_kb_event_t *xkey = (X_kb_event_t *)event;
    tank_rt_t *tank_rt = (tank_rt_t *)arg;
    redraw_man_t *rdman;
    mb_tman_t *tman;
    mb_word_t *word;
    ob_factory_t *factory;
    subject_t *comp_sub;
    mb_timeval_t start_tm, playing, now;

    if(tank_rt->tank1_progm != NULL)
	return;

    rdman = X_MB_rdman(tank_rt->mb_rt);
    tman = X_MB_tman(tank_rt->mb_rt);

    tank_rt->tank1_progm = mb_progm_new(2, rdman);
    comp_sub = mb_progm_get_complete(tank_rt->tank1_progm);
    factory = rdman_get_ob_factory(rdman);
    subject_add_observer(factory, comp_sub,
			 free_progm_handler,
			 tank_rt);

    MB_TIMEVAL_SET(&start_tm, 0, 0);
    MB_TIMEVAL_SET(&playing, 0, 500000);
    word = mb_progm_next_word(tank_rt->tank1_progm,
			      &start_tm, &playing);

    switch(xkey->sym) {
    case 0xff51:		/* left */
	mb_shift_new(-50, 0, tank_rt->tank1->root_coord, word);
	break;

    case 0xff52:		/* up */
	mb_shift_new(0, -50, tank_rt->tank1->root_coord, word);
	break;

    case 0xff53:		/* right */
	mb_shift_new(50, 0, tank_rt->tank1->root_coord, word);
	break;

    case 0xff54:		/* down */
	mb_shift_new(0, 50, tank_rt->tank1->root_coord, word);
	break;

    case 0x20:			/* space */
    case 0xff0d:		/* enter */
	break;
    }
    get_now(&now);
    mb_progm_start(tank_rt->tank1_progm, tman, &now);
}

static void init_keyboard(tank_rt_t *tank_rt) {
    X_MB_runtime_t *mb_rt;
    subject_t *kbevents;
    redraw_man_t *rdman;
    ob_factory_t *factory;

    mb_rt = tank_rt->mb_rt;
    kbevents = X_MB_kbevents(mb_rt);

    rdman = X_MB_rdman(mb_rt);
    factory = rdman_get_ob_factory(rdman);

    tank_rt->kb_observer =
	subject_add_observer(factory, kbevents, keyboard_handler, tank_rt);
}

void
initial_tank(tank_rt_t *tank_rt, X_MB_runtime_t *mb_rt) {
    redraw_man_t *rdman;
    mb_tman_t *tman;
    mud_t *mud;
    brick_t *brick;
    rock_t *rock;
    bush_t *bush;
    mb_word_t *word;
    mb_timeval_t start, playing;
    mb_timeval_t mbtv;
    subject_t *comp_sub;
    ob_factory_t *factory;
    int i, j;

    rdman = X_MB_rdman(mb_rt);

    tank_rt->mb_rt = mb_rt;
    for(i = 0; i < 12; i++) {
	for(j = 0; j < 16; j++) {
	    switch(map[i][j]) {
	    case MUD:
		mud = mud_new(rdman);
		CHANGE_POS(mud, j * 50, i * 50);
		tank_rt->map[i][j] = (void *)mud;
		break;
	    case BRI:
		brick = brick_new(rdman);
		CHANGE_POS(brick, j * 50, i * 50);
		tank_rt->map[i][j] = (void *)brick;
		break;
	    case ROC:
		rock = rock_new(rdman);
		CHANGE_POS(rock, j * 50, i * 50);
		tank_rt->map[i][j] = (void *)rock;
		break;
	    case BSH:
		bush = bush_new(rdman);
		CHANGE_POS(bush, j * 50, i * 50);
		tank_rt->map[i][j] = (void *)bush;
		break;
	    }
	}
    }

    tank_rt->tank1 = tank1_new(rdman);
    CHANGE_POS(tank_rt->tank1, 5 * 50, 11 * 50);
    tank_rt->tank2 = tank2_new(rdman);
    CHANGE_POS(tank_rt->tank2, 10 * 50, 11 * 50);
    for(i = 0; i < 3; i++) {
	tank_rt->tank_enemies[i] = tank_en_new(rdman);
	CHANGE_POS(tank_rt->tank_enemies[i], (2 + i * 3) * 50, 0);
    }
    tank_rt->n_enemy = i;

    tank_rt->tank1_progm = mb_progm_new(4, rdman);

    MB_TIMEVAL_SET(&start, 1, 0);
    MB_TIMEVAL_SET(&playing, 3, 0);
    word = mb_progm_next_word(tank_rt->tank1_progm, &start, &playing);

    mb_shift_new(0, -150, tank_rt->tank1->root_coord, word);
    mb_shift_new(0, -150, tank_rt->tank2->root_coord, word);

    MB_TIMEVAL_SET(&start, 5, 0);
    MB_TIMEVAL_SET(&playing, 3, 0);
    word = mb_progm_next_word(tank_rt->tank1_progm, &start, &playing);

    mb_shift_new(0, 150, tank_rt->tank1->root_coord, word);
    mb_shift_new(0, 150, tank_rt->tank2->root_coord, word);

    /* Free program after program completed. */
    comp_sub = mb_progm_get_complete(tank_rt->tank1_progm);
    factory = rdman_get_ob_factory(rdman);
    subject_add_observer(factory, comp_sub, free_progm_handler, tank_rt);

    tman = X_MB_tman(mb_rt);
    get_now(&mbtv);
    mb_progm_start(tank_rt->tank1_progm, tman, &mbtv);

    init_keyboard(tank_rt);
}

int
main(int argc, char *const argv[]) {
    X_MB_runtime_t *rt;
    tank_rt_t tank_rt;

    rt = X_MB_new(":0.0", 800, 600);

    initial_tank(&tank_rt, rt);
    
    X_MB_handle_connection(rt);

    X_MB_free(rt);
}
