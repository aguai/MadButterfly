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
};

#define CHANGE_POS(g, x, y) do {			\
	(g)->root_coord->matrix[0] = 1.0;		\
	(g)->root_coord->matrix[2] = x;			\
	(g)->root_coord->matrix[4] = 1.0;		\
	(g)->root_coord->matrix[5] = y;			\
	rdman_coord_changed(rdman, (g)->root_coord);	\
    } while(0)

void
initial_tank(tank_rt_t *tank_rt, X_MB_runtime_t *mb_rt) {
    redraw_man_t *rdman;
    mud_t *mud;
    brick_t *brick;
    rock_t *rock;
    bush_t *bush;
    int i, j;

    rdman = mb_rt->rdman;

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
}

int
main(int argc, char *const argv[]) {
    X_MB_runtime_t rt;
    tank_rt_t tank_rt;

    X_MB_init(":0.0", 800, 600, &rt);

    initial_tank(&tank_rt, &rt);
    
    X_MB_handle_connection(&rt);

    X_MB_destroy(&rt);
}
