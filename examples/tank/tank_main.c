#include <sys/time.h>
#include <string.h>
#include <mb/mb.h>
#include <mb/tools.h>
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

#define MAP_W 16
#define MAP_H 12

/*! \defgroup tank_elf Tank Elf
 * \brief Tank elf module provides control functions of tanks in game.
 * @{
 */
struct _tank {
    coord_t *coord_pos;		/*!< \brief coordinate for position */
    coord_t *coord_rot;		/*!< \brief coordinate for rotation */
    int map_x, map_y;
    int direction;
    mb_progm_t *progm;
};
typedef struct _tank tank_t;
enum { TD_UP = 0, TD_RIGHT, TD_DOWN, TD_LEFT };


/* @} */

typedef struct _tank_rt tank_rt_t;

struct _tank_rt {
    tank_t *tank1;
    tank1_t *tank1_o;
    tank_t *tank2;
    tank2_t *tank2_o;
    int n_enemy;
    tank_t *tank_enemies[10];
    tank_en_t *tank_enemies_o[10];
    tank_t *tanks[12];
    int n_tanks;
    void *map[12][16];
    X_MB_runtime_t *mb_rt;
    observer_t *kb_observer;
};

/*! \ingroup tank_elf
 * @{
 */
static tank_t *tank_new(coord_t *coord_pos,
			coord_t *coord_rot,
			int map_x, int map_y,
			X_MB_runtime_t *mb_rt) {
    tank_t *tank;
    redraw_man_t *rdman;

    tank = O_ALLOC(tank_t);
    if(tank == NULL)
	return NULL;

    rdman = X_MB_rdman(mb_rt);

    tank->coord_pos = coord_pos;
    tank->coord_rot = coord_rot;
    tank->map_x = map_x;
    tank->map_y = map_y;
    tank->direction = TD_UP;
    tank->progm = NULL;

    memset(coord_pos->matrix, 0, sizeof(co_aix[6]));
    coord_pos->matrix[0] = 1;
    coord_pos->matrix[2] = map_x * 50;
    coord_pos->matrix[4] = 1;
    coord_pos->matrix[5] = map_y * 50;
    rdman_coord_changed(rdman, coord_pos);

    return tank;
}

static void tank_free(tank_t *tank, X_MB_runtime_t *xmb_rt) {
    mb_tman_t *tman;

    if(tank->progm) {
	tman = X_MB_tman(xmb_rt);
	mb_progm_abort(tank->progm);
    }
    free(tank);
}

/*! \brief Clean program for a tank.
 *
 * It is called when the program is completed.
 */
static void clean_tank_progm_handler(event_t *event, void *arg) {
    tank_t *tank = (tank_t *)arg;

    mb_progm_free(tank->progm);
    tank->progm = NULL;
}

#define PI 3.1415926

static void tank_move(tank_t *tank, int direction,
		      tank_rt_t *tank_rt) {
    X_MB_runtime_t *xmb_rt = tank_rt->mb_rt;
    redraw_man_t *rdman;
    mb_tman_t *tman;
    ob_factory_t *factory;
    /* for the program */
    mb_progm_t *progm;
    subject_t *comp_sub;
    mb_word_t *word;
    mb_timeval_t start, playing;
    mb_timeval_t now;
    /* for position */
    co_aix sh_x, sh_y;
    /* for direction */
    float ang1, ang2;
    float rot_diff;
    int i;
    static co_aix shift_xy[][2] = {{0, -50}, {50, 0}, {0, 50}, {-50, 0}};
    static int map_shift[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
    static float angles[4] = {0, PI / 2, PI , PI * 3 / 2};
    static float rotations[7] = {PI / 2, PI , -PI / 2,
				 0, PI / 2, PI , -PI / 2};

    if(tank->progm != NULL)
	return;

    /*
     * Keep inside the map.
     */
    if(direction == tank->direction) {
	switch(direction) {
	case TD_UP:
	    if(tank->map_y == 0)
		return;
	    if(map[tank->map_y - 1][tank->map_x] != MUD)
		return;
	    break;
	case TD_RIGHT:
	    if(tank->map_x >= (MAP_W - 1))
		return;
	    if(map[tank->map_y][tank->map_x + 1] != MUD)
		return;
	    break;
	case TD_DOWN:
	    if(tank->map_y >= (MAP_H - 1))
		return;
	    if(map[tank->map_y + 1][tank->map_x] != MUD)
		return;
	    break;
	case TD_LEFT:
	    if(tank->map_x == 0)
		return;
	    if(map[tank->map_y][tank->map_x - 1] != MUD)
		return;
	    break;
	}

	tank->map_x += map_shift[direction][0];
	tank->map_y += map_shift[direction][1];
	for(i = 0; i < tank_rt->n_tanks; i++) {
	    if(tank != tank_rt->tanks[i] &&
	       tank->map_x == tank_rt->tanks[i]->map_x &&
	       tank->map_y == tank_rt->tanks[i]->map_y) {
		tank->map_x -= map_shift[direction][0];
		tank->map_y -= map_shift[direction][1];
		return;
	    }
	}
    }

    rdman = X_MB_rdman(xmb_rt);
    tman = X_MB_tman(xmb_rt);
    factory = X_MB_ob_factory(xmb_rt);

    progm = mb_progm_new(1, rdman);
    tank->progm = progm;

    MB_TIMEVAL_SET(&start, 0, 0);
    MB_TIMEVAL_SET(&playing, 0, 500000);
    word = mb_progm_next_word(progm, &start, &playing);

    if(direction == tank->direction) {
	/* Shift/move */
	sh_x = shift_xy[direction][0];
	sh_y = shift_xy[direction][1];
	mb_shift_new(sh_x, sh_y, tank->coord_pos, word);
    } else {
	/* Change direction */
	rot_diff = rotations[3 - tank->direction + direction];
	ang1 = angles[tank->direction];
	ang2 = ang1 + rot_diff;
	mb_rotate_new(ang1, ang2, tank->coord_rot, word);
	tank->direction = direction;
    }

    /* Clean program when it is completed. */
    comp_sub = mb_progm_get_complete(progm);
    subject_add_observer(factory, comp_sub,
			 clean_tank_progm_handler, tank);

    get_now(&now);
    mb_progm_start(progm, tman, &now);
}

/* @} */

#define CHANGE_POS(g, x, y) do {			\
	(g)->root_coord->matrix[0] = 1.0;		\
	(g)->root_coord->matrix[2] = x;			\
	(g)->root_coord->matrix[4] = 1.0;		\
	(g)->root_coord->matrix[5] = y;			\
	rdman_coord_changed(rdman, (g)->root_coord);	\
    } while(0)

static void keyboard_handler(event_t *event, void *arg) {
    X_kb_event_t *xkey = (X_kb_event_t *)event;
    tank_rt_t *tank_rt = (tank_rt_t *)arg;
    int direction;

    if(xkey->event.type != EVT_KB_PRESS)
	return;

    switch(xkey->sym) {
    case 0xff51:		/* left */
	direction = TD_LEFT;
	tank_move(tank_rt->tank1, direction, tank_rt);
	break;

    case 0xff52:		/* up */
	direction = TD_UP;
	tank_move(tank_rt->tank1, direction, tank_rt);
	break;

    case 0xff53:		/* right */
	direction = TD_RIGHT;
	tank_move(tank_rt->tank1, direction, tank_rt);
	break;

    case 0xff54:		/* down */
	direction = TD_DOWN;
	tank_move(tank_rt->tank1, direction, tank_rt);
	break;

    case 0x20:			/* space */
	break;
    case 0xff0d:		/* enter */
    default:
	return;
    }

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

/*! \brief Make coord objects for elfs (tanks). */
static void make_elf_coords(redraw_man_t *rdman, coord_t **coord_pos,
			    coord_t **coord_rot, coord_t **coord_center) {
    coord_t *coord_back;

    *coord_pos = rdman_coord_new(rdman, rdman->root_coord);

    coord_back = rdman_coord_new(rdman, *coord_pos);
    coord_back->matrix[2] = 25;
    coord_back->matrix[5] = 25;
    rdman_coord_changed(rdman, coord_back);

    *coord_rot = rdman_coord_new(rdman, coord_back);

    *coord_center = rdman_coord_new(rdman, *coord_rot);
    (*coord_center)->matrix[2] = -25;
    (*coord_center)->matrix[5] = -25;
    rdman_coord_changed(rdman, *coord_center);
}

void
initial_tank(tank_rt_t *tank_rt, X_MB_runtime_t *mb_rt) {
    redraw_man_t *rdman;
    /* for map areas */
    mud_t *mud;
    brick_t *brick;
    rock_t *rock;
    bush_t *bush;
    /* for tanks */
    coord_t *coord_center, *coord_pos, *coord_rot;
    tank1_t *tank1_o;
    tank2_t *tank2_o;
    tank_en_t *tank_en_o;
    int i, j;

    rdman = X_MB_rdman(mb_rt);

    tank_rt->mb_rt = mb_rt;
    for(i = 0; i < 12; i++) {
	for(j = 0; j < 16; j++) {
	    switch(map[i][j]) {
	    case MUD:
		mud = mud_new(rdman, rdman->root_coord);
		CHANGE_POS(mud, j * 50, i * 50);
		tank_rt->map[i][j] = (void *)mud;
		break;
	    case BRI:
		brick = brick_new(rdman, rdman->root_coord);
		CHANGE_POS(brick, j * 50, i * 50);
		tank_rt->map[i][j] = (void *)brick;
		break;
	    case ROC:
		rock = rock_new(rdman, rdman->root_coord);
		CHANGE_POS(rock, j * 50, i * 50);
		tank_rt->map[i][j] = (void *)rock;
		break;
	    case BSH:
		bush = bush_new(rdman, rdman->root_coord);
		CHANGE_POS(bush, j * 50, i * 50);
		tank_rt->map[i][j] = (void *)bush;
		break;
	    }
	}
    }

    make_elf_coords(rdman, &coord_pos, &coord_rot, &coord_center);
    tank1_o = tank1_new(rdman, coord_center);
    tank_rt->tank1 = tank_new(coord_pos, coord_rot, 5, 11, mb_rt);
    tank_rt->tank1_o = tank1_o;

    make_elf_coords(rdman, &coord_pos, &coord_rot, &coord_center);
    tank2_o = tank2_new(rdman, coord_center);
    tank_rt->tank2 = tank_new(coord_pos, coord_rot, 10, 11, mb_rt);
    tank_rt->tank2_o = tank2_o;

    for(i = 0; i < 3; i++) {
	make_elf_coords(rdman, &coord_pos, &coord_rot, &coord_center);
	tank_en_o = tank_en_new(rdman, coord_center);
	tank_rt->tank_enemies[i] = tank_new(coord_pos, coord_rot,
					    i * 3 + 3, 0, mb_rt);
	tank_rt->tank_enemies_o[i] = tank_en_o;
	tank_rt->tanks[i] = tank_rt->tank_enemies[i];
    }
    tank_rt->n_enemy = i;

    tank_rt->tanks[i++] =tank_rt->tank1;
    tank_rt->tanks[i++] =tank_rt->tank2;
    tank_rt->n_tanks = i;

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
