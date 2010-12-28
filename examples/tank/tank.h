#ifndef __TANK_H_
#define __TANK_H_

#include <mb.h>
#include "svgs.h"

/*! \ingroup tank
 * @{
 */
/*! \brief Tile types in a map. */
enum { MUD, ROC, BRI, BSH };

/*! \brief Map of the game. */
extern char map[12][16];

#define MAP_W 16
#define MAP_H 12
/* @} */

/*! \defgroup bullet_elf Bullet Elf
 * \ingroup tank
 * @{
 */
/*! \brief Information about bullet elf
 */
struct _tank_bullet {
    redraw_man_t *rdman;
    coord_t *coord_pos;
    coord_t *coord_rot;
    bullet_t *bullet_obj;
    int start_map_x, start_map_y;
    int direction;
    mb_progm_t *progm;
    mb_timeval_t start_time;
    observer_t *observer_redraw;
    int hit_tmr;
    mb_timer_man_t *timer_man;
};
typedef struct _tank_bullet tank_bullet_t;
/*! \brief The direction a bullet is going.
 */
enum { BU_UP = 0, BU_RIGHT, BU_DOWN, BU_LEFT };
/* @} */

/*! \defgroup tank_elf Tank Elf
 * \brief Tank elf module provides control functions of tanks in game.
 * \ingroup tank
 * @{
 */
/*! \brief Information about a tank elf. */
struct _tank {
    coord_t *coord_pos;		/*!< \brief coordinate for position */
    coord_t *coord_rot;		/*!< \brief coordinate for rotation */
    coord_t *coord_center;
    int map_x, map_y;
    int direction;
    mb_progm_t *progm;
    tank_bullet_t *bullet;
    struct _tank_rt *tank_rt;	/*!< \brief for bullet to check
				 * hitting on tanks.
				 */
};
typedef struct _tank tank_t;
enum { TD_UP = 0, TD_RIGHT, TD_DOWN, TD_LEFT };

/* @} */

/*
 * \ingroup tank
 * @{
 */
typedef struct _tank_rt tank_rt_t;

/*! \brief Runtime information for tank, this game/example.
 */
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
    mb_rt_t *mb_rt;
    observer_t *kb_observer;
};

extern void tank_move(tank_t *tank, int direction, tank_rt_t *tank_rt);
extern void tank_fire_bullet(tank_rt_t *tank_rt, tank_t *tank);

/* From enemy.c */
extern void init_enemies(tank_rt_t *tank_rt);

/* @} */

#endif /* __TANK_H_ */
