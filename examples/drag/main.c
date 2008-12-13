/*! \file
 *
 * svg2code_ex is an example that show programmers how to create a
 * menu with MadButterfly.
 *
 */
#include <stdio.h>
#include <mb.h>
#include "menu.h"

typedef struct _engine engine_t;
struct _engine {
    X_MB_runtime_t *rt;
    redraw_man_t *rdman;
    menu_t *menu;
    int state;
    co_aix orx,ory;
    int start_x,start_y;
    coord_t *cursor;
};
engine_t *engine_init()
{

    X_MB_runtime_t *rt;
    rt = X_MB_new(":0.0", 800, 600);
    engine_t *en = (engine_t *) malloc(sizeof(engine_t));

    en->rt = rt;
    en->rdman =  X_MB_rdman(rt);
    en->state = 0;
    return en;
}
void engine_close(engine_t *en)
{
    /*
     * Start handle connections, includes one to X server.
     * User start to interact with the application.
     */
    X_MB_handle_connection(en->rt);

    /*
     * Clean
     */
    menu_free(en->menu);
    X_MB_free(en->rt);
    free(en);
}
#define COORD_SHOW(group) coord_show(group);rdman_coord_changed(X_MB_rdman(ex_rt->rt), group)
#define COORD_HIDE(group) coord_hide(group);rdman_coord_changed(X_MB_rdman(ex_rt->rt), group)


void coord_move(coord_t *c, co_aix x, co_aix y)
{
    c->matrix[2] = x;
    c->matrix[5] = y;
}


static void cursor_press_handler(event_t *evt, void *arg) {
    engine_t *en = (engine_t *) arg;
    mouse_event_t *mev = (mouse_event_t *) evt;

    en->start_x = mev->x;
    en->start_y = mev->y;
    en->orx = en->cursor->matrix[2];
    en->ory = en->cursor->matrix[5];
    printf("pressed %g %g\n",en->orx,en->ory);
    en->state = 1;
}

static void cursor_release_handler(event_t *evt, void *arg) {
    engine_t *en = (engine_t *) arg;
    printf("up\n");
    en->state = 0;
}

static void cursor_move_handler(event_t *evt, void *arg) {
    engine_t *en = (engine_t *) arg;
    mouse_event_t *mev = (mouse_event_t *) evt;

    if (en->state) {
        printf("move to (%d %d)\n", mev->x,mev->y);
        coord_move(en->cursor,en->orx + (mev->x-en->start_x),en->ory + (mev->y-en->start_y));
        rdman_coord_changed(en->rdman, en->cursor);
        /* Update changed part to UI. */
        rdman_redraw_changed(en->rdman);
    }
}

int main(int argc, char * const argv[]) {
    subject_t *subject;
    engine_t *en;

    en = engine_init();
    en->menu = menu_new(en->rdman, en->rdman->root_coord);
    en->cursor = (coord_t *) MB_SPRITE_GET_OBJ(&en->menu->lsym.sprite, "star");
    printf("en->cursor=%x star=%x\n",en->cursor,en->menu->star);
    printf("sprite=%x\n",&en->menu->lsym.sprite);
    printf("en->menu=%x\n",en->menu);

    /*
     * Register observers to subjects of events for objects.
     */
    subject = coord_get_mouse_event(en->cursor);
    subject_add_event_observer(subject,  EVT_MOUSE_BUT_PRESS, cursor_press_handler, en);
    subject_add_event_observer(subject,  EVT_MOUSE_BUT_RELEASE, cursor_release_handler, en);
    subject_add_event_observer(subject,  EVT_MOUSE_MOVE, cursor_move_handler, en);


    engine_close(en);

    return 0;
}

/* vim: set ts=4 */
