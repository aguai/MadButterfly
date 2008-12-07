/*! \file
 *
 * svg2code_ex is an example that show programmers how to create a
 * menu with MadButterfly.
 *
 */
#include <stdio.h>
#include <mb.h>
#include "menu.h"
#include "button.h"


typedef struct _engine engine_t;
struct _engine {
    X_MB_runtime_t *rt;
    redraw_man_t *rdman;
    menu_t *menu;
    button_t *button;
    int state;
    co_aix orx,ory;
    int start_x,start_y;
    observer_t *obs1,*obs2;
    shape_t *rect;
    co_aix rx,ry;
};
engine_t *engine_init()
{

    X_MB_runtime_t *rt;
    rt = X_MB_new(":0.0", 800, 600);
    engine_t *en = (engine_t *) malloc(sizeof(engine_t));

    en->rt = rt;
    en->rdman =  X_MB_rdman(rt);
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
#define COORD_SHOW(group) coord_show(group);rdman_coord_changed(en->rdman, group)
#define COORD_HIDE(group) coord_hide(group);rdman_coord_changed(en->rdman, group)

#define CMOUSE(e) (coord_get_mouse_event(e))



static void button_move(event_t *evt, void *arg) 
{
    engine_t *en = (engine_t *) arg;

    
    printf("Mouse move\n");
    COORD_SHOW(en->button->active);
    rdman_coord_changed(en->rdman,en->button->root_coord);
    rdman_redraw_changed(en->rdman);
}
static void button_out(event_t *evt, void *arg) 
{
    engine_t *en = (engine_t *) arg;

    printf("mouse out\n");
    COORD_HIDE(en->button->active);
    rdman_coord_changed(en->rdman,en->button->root_coord);
    rdman_redraw_changed(en->rdman);
}

void button_pressed(event_t *evt, void *arg)
{
	printf("Pressed\n");
}

void engine_add_button(engine_t *en, coord_t *normal, coord_t *active, void (*func)())
{
    active->matrix[2] = 200;
    active->matrix[5] = 200;
    normal->matrix[2] = 200;
    normal->matrix[5] = 200;
    COORD_HIDE(en->button->active);
    rdman_coord_changed(en->rdman,en->button->root_coord);
    rdman_redraw_changed(en->rdman);
    subject_add_event_observer(CMOUSE(normal), EVT_MOUSE_MOVE, button_move,en);
    subject_add_event_observer(CMOUSE(active), EVT_MOUSE_OUT, button_out,en);
    subject_add_event_observer(CMOUSE(active), EVT_MOUSE_BUT_RELEASE, button_pressed,en);
}


void coord_move(coord_t *c, co_aix x, co_aix y)
{
    c->matrix[2] = x;
    c->matrix[5] = y;
}



static void add_rect_move(event_t *evt, void *arg) 
{
    engine_t *en = (engine_t *) arg;
    mouse_event_t *mev = (mouse_event_t *) evt;

    printf("resize rectangle\n");
    sh_rect_set(en->rect, en->start_x, en->start_y, mev->x - en->start_x, mev->y-en->start_y,en->rx,en->ry);
    rdman_shape_changed(en->rdman,en->rect);
    rdman_redraw_changed(en->rdman);
}

static void add_rect_release(event_t *evt, void *arg) 
{
    engine_t *en = (engine_t *) arg;
    mouse_event_t *mev = (mouse_event_t *) evt;

    printf("rectangle done\n");
    subject_remove_observer(CMOUSE(en->rdman->root_coord), en->obs1);
    subject_remove_observer(CMOUSE(en->rdman->root_coord), en->obs2);
}

static void add_rect_2(event_t *evt, void *arg) 
{
    engine_t *en = (engine_t *) arg;
    mouse_event_t *mev = (mouse_event_t *) evt;
    paint_t *color;

    printf("select first point\n");
    // Add an rect path 

    en->start_x = mev->x;
    en->start_y = mev->y;
    subject_remove_observer(CMOUSE(en->rdman->root_coord), en->obs1);
    subject_remove_observer(CMOUSE(en->rdman->root_coord), en->obs2);
    en->obs1 = subject_add_event_observer(CMOUSE(en->rdman->root_coord), EVT_MOUSE_MOVE, add_rect_move, en);
    en->obs2 = subject_add_event_observer(CMOUSE(en->rdman->root_coord), EVT_MOUSE_BUT_RELEASE, add_rect_release, en);
}

static void add_rect_2_move(event_t *evt, void *arg) 
{
    engine_t *en = (engine_t *) arg;
    mouse_event_t *mev = (mouse_event_t *) evt;

    sh_rect_set(en->rect, mev->x, mev->y, 50,50,en->rx,en->ry);
    rdman_shape_changed(en->rdman,en->rect);
    rdman_redraw_changed(en->rdman);
}

static void add_rect(event_t *evt, void *arg) 
{
    engine_t *en = (engine_t *) arg;
    mouse_event_t *mev = (mouse_event_t *) evt;
    paint_t *color;

    printf("menut selected\n");
    en->obs1 = subject_add_event_observer(CMOUSE(en->rdman->root_coord), EVT_MOUSE_BUT_PRESS, add_rect_2, en);
    en->obs2 = subject_add_event_observer(CMOUSE(en->rdman->root_coord), EVT_MOUSE_MOVE, add_rect_2_move, en);
    en->rect = rdman_shape_rect_new(en->rdman, mev->x, mev->y, 50 , 50, en->rx, en->ry);
    // Paint it with color
    color = rdman_paint_color_new(en->rdman, 0.800000, 0.800000, 0.400000, 1.000000);
    rdman_paint_fill(en->rdman, color, en->rect);
    // Add to the stage
    rdman_add_shape(en->rdman, en->rect, en->menu->root_coord);
}





int main(int argc, char * const argv[]) {
    subject_t *subject;
    engine_t *en;

    en = engine_init();
    en->menu = menu_new(en->rdman, en->rdman->root_coord);
    en->button = button_new(en->rdman, en->rdman->root_coord);
    engine_add_button(en, en->button->normal, en->button->active, button_pressed);

    en->rx = 0;
    en->ry = 0;

    /*
     * Register observers to subjects of events for objects.
     */
    subject = coord_get_mouse_event(en->menu->rect);
    subject_add_event_observer(subject,  EVT_MOUSE_BUT_RELEASE, add_rect, en);


    engine_close(en);

    return 0;
}

/* vim: set ts=4 */
