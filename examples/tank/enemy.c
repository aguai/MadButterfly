#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "tank.h"

struct _enemy {
    tank_rt_t *tank_rt;
    tank_t *tank;
    int memory[4];
};
typedef struct _enemy enemy_t;

/* \brief Fire and move to the target tank if they are in a row or column.
 */
static int
try_fire(tank_t *me, tank_t *target, tank_rt_t *tank_rt) {
    int x, y;
    int *tracer = NULL;
    int target_value;
    int target_dir;
    
    if(me->map_x == target->map_x) { /* In a row */
	tracer = &y;
	target_value = target->map_y;
	if(me->map_y < target->map_y)
	    target_dir = TD_DOWN;
	else
	    target_dir = TD_UP;
    }
    if(me->map_y == target->map_y) { /* In a column */
	tracer = &x;
	target_value = target->map_x;
	if(me->map_x < target->map_x)
	    target_dir = TD_RIGHT;
	else
	    target_dir = TD_LEFT;
    }

    if(tracer == NULL)
	return 0;		/* Not in a row or column */
    
    /* Check obstacles between tanks */
    x = me->map_x;
    y = me->map_y;
    if(*tracer < target_value) {
	while(++*tracer < target_value) {
	    if(map[y][x] != MUD)
		break;
	}
    } else {
	while(--*tracer > target_value) {
	    if(map[y][x] != MUD)
		break;
	}
    }
    
     if(*tracer == target_value) { /* No any obstacle between tanks */
	 /* Fire and move to target */
	if(me->direction == target_dir && me->bullet == NULL)
	    tank_fire_bullet(tank_rt, me);
	tank_move(me, target_dir, tank_rt);
	return 1;
    }

    return 0;			/* Find one or more obstacles */
}

#define NOTHING 0
#define SOMETHING 1

static void
move_tank(enemy_t *enemy, tank_rt_t *tank_rt) {
    tank_t *me;
    tank_t **tanks, *tank;
    static const int shift_xy[4][2] = {
	{1, 0}, {0, 1},
	{-1, 0}, {0, -1}};
    int status[4];
    int x, y;
    int i, tank_i;
    int dir, chk_dir;
    int possibles;
    int which_dir;
    
    me = enemy->tank;
    tanks = tank_rt->tanks;
    
    /* Collect status */
    for(i = 0; i < 4; i++) {
	x = me->map_x + shift_xy[i][0];
	y = me->map_y + shift_xy[i][1];
	
	/* Check obstacles */
	if(x == -1 || y == -1 || x >= MAP_W || y >= MAP_H) {
	    /* Out of range */
	    status[i] = SOMETHING;
	    continue;
	}
	if(map[y][x] == MUD)
	    status[i] = NOTHING;
	else
	    status[i] = SOMETHING;
	
	/* Check tanks */
	for(tank_i = 0; tank_i < tank_rt->n_tanks; tank_i++) {
	    tank = tanks[tank_i];
	    if(tank->map_x == x && tank->map_y == y) {
		status[i] = SOMETHING;
		break;
	    }
	}
    }

    /* Try the same direction if status is not changed. */
    for(i = 0; i < 4; i++) {
	if(status[i] != enemy->memory[i])
	    break;
    }
    if(i == 4) {		/* Status is not changed */
	tank_move(me, me->direction, tank_rt);
	return;
    }

    memcpy(enemy->memory, status, sizeof(int) * 4);

    /* Check possible directions except backward. */
    switch(me->direction) {
    case TD_UP:
	dir = 3;
	break;
    case TD_RIGHT:
	dir = 0;
	break;
    case TD_DOWN:
	dir = 1;
	break;
    case TD_LEFT:
	dir = 2;
	break;
    }

    possibles = 0;
    for(i = 0; i < 3; i++) {
	chk_dir = (dir - 1  + i) % 4;
	if(status[chk_dir] == NOTHING)
	    possibles++;
    }

    if(possibles == 0) {	/* Only can move backward */
	switch(me->direction) {
	case TD_UP:
	    tank_move(me, TD_DOWN, tank_rt);
	    break;
	case TD_RIGHT:
	    tank_move(me, TD_LEFT, tank_rt);
	    break;
	case TD_DOWN:
	    tank_move(me, TD_UP, tank_rt);
	    break;
	case TD_LEFT:
	    tank_move(me, TD_RIGHT, tank_rt);
	    break;
	}
	return;
    }
    
    which_dir = rand() % possibles;
    for(i = 0; i < 3; i++) {
	chk_dir = (dir - 1  + i) % 4;
	if(status[chk_dir] == NOTHING) {
	    if(which_dir == 0)
		break;
	    which_dir--;
	}
    }
    switch(chk_dir) {
    case 0:
	tank_move(me, TD_RIGHT, tank_rt);
	break;
    case 1:
	tank_move(me, TD_DOWN, tank_rt);
	break;
    case 2:
	tank_move(me, TD_LEFT, tank_rt);
	break;
    case 3:
	tank_move(me, TD_UP, tank_rt);
	break;
    }
}

static void
_drive_enemy_tank(enemy_t *enemy) {
    tank_rt_t *tank_rt;
    tank_t *me, *tank1, *tank2;
    int r;
    
    tank_rt = enemy->tank_rt;
    tank1 = tank_rt->tank1;
    tank2 = tank_rt->tank2;
    me = enemy->tank;
    
    r = try_fire(me, tank1, tank_rt);
    if(r)
	return;
    r = try_fire(me, tank2, tank_rt);
    if(r)
	return;

    move_tank(enemy, tank_rt);
}

static enemy_t *enemies = NULL;
static mb_timer_man_t *timer_man;

/*! \brief Drive every enemy tanks.
 */
static void
enemy_tank_driver(int hdl, const mb_timeval_t *tmo,
		  const mb_timeval_t *now, void *data) {
    tank_rt_t *tank_rt = (tank_rt_t *)data;
    enemy_t *enemy;
    int n_enemy;
    mb_timeval_t timeout, addend;
    int i;
    
    n_enemy = tank_rt->n_enemy;
    for(i = 0; i < n_enemy; i++) {
	enemy = enemies + i;
	if(enemy->tank->progm == NULL)
	    _drive_enemy_tank(enemy);
    }
    
    get_now(&timeout);
    MB_TIMEVAL_SET(&addend, 0, 300000);
    MB_TIMEVAL_ADD(&timeout, &addend);
    mb_timer_man_timeout(timer_man, &timeout, enemy_tank_driver, tank_rt);
}

/*! \brief Start a timer for enemy tank driver.
 */
static void
start_enemy_tank_timer(tank_rt_t *tank_rt) {
    mb_timeval_t timeout, addend;
    
    timer_man = mb_runtime_timer_man(tank_rt->mb_rt);
    
    get_now(&timeout);
    MB_TIMEVAL_SET(&addend, 0, 300000);
    MB_TIMEVAL_ADD(&timeout, &addend);
    mb_timer_man_timeout(timer_man, &timeout, enemy_tank_driver, tank_rt);
}

void
init_enemies(tank_rt_t *tank_rt) {
    int n_enemy;
    tank_t **tank_enemies;
    tank_t *tank;
    int i, j;
    
    n_enemy = tank_rt->n_enemy;
    tank_enemies = tank_rt->tank_enemies;

    enemies = (enemy_t *)malloc(sizeof(enemy_t) * n_enemy);

    for(i = 0; i < n_enemy; i++) {
	tank = tank_enemies[i];
	enemies[i].tank_rt = tank_rt;
	enemies[i].tank = tank;
	for(j = 0; j < 4; j++)
	    enemies[i].memory[j] = SOMETHING;
    }

    srand(time(NULL));
    start_enemy_tank_timer(tank_rt);
}
