/*! \file
 *
 * svg2code_ex is an example that show programmers how to create a
 * menu with MadButterfly.
 *
 */
#include <stdio.h>
#include <mb.h>
#include <string.h>
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


typedef struct _mb_button {
    mb_obj_t obj;
    engine_t *en;
    coord_t *root;
    coord_t *active;
    coord_t *normal;
    coord_t *click;
    void (*press)();
    void *arg;
    observer_t *obs_move,*obs_out,*obs_press;
    mb_progm_t *progm;
} mb_button_t;


#define COORD_SHOW(group) coord_show(group);rdman_coord_changed(en->rdman, group)
#define COORD_HIDE(group) coord_hide(group);rdman_coord_changed(en->rdman, group)

#define CMOUSE(e) (coord_get_mouse_event(e))


static void mb_button_pressed(event_t *evt, void *arg);
static void mb_button_out(event_t *evt, void *arg);

static void mb_button_move(event_t *evt, void *arg) 
{
    mb_button_t *btn = (mb_button_t *) arg;
    engine_t *en = btn->en;

    
    printf("Mouse move\n");
    COORD_SHOW(btn->active);
#if 0
    rdman_coord_changed(btn->en->rdman,btn->root);
#endif
    rdman_redraw_changed(btn->en->rdman);
}
static void mb_button_out(event_t *evt, void *arg) 
{
    mb_button_t *btn = (mb_button_t *) arg;
    engine_t *en = btn->en;

    if (btn->progm) {
	    mb_progm_abort(btn->progm);
	    btn->progm = NULL;
    }
    printf("mouse out\n");
    COORD_HIDE(btn->click);
    COORD_HIDE(btn->active);
    COORD_SHOW(btn->normal);
#if 1
    rdman_coord_changed(btn->en->rdman,btn->normal);
#endif
    rdman_redraw_changed(btn->en->rdman);
}

void mb_button_show_active(event_t *evt, void *arg)
{
    mb_button_t *btn = (mb_button_t *) arg;
    engine_t *en = btn->en;

    COORD_SHOW(btn->active);
    rdman_coord_changed(btn->en->rdman,btn->root);
    rdman_redraw_changed(btn->en->rdman);
}

void mb_button_pressed(event_t *evt, void *arg)
{
    mb_button_t *btn = (mb_button_t *) arg;
    engine_t *en = btn->en;
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;

    printf("Pressed\n");
    if (btn->progm) {
	    mb_progm_abort(btn->progm);
	    btn->progm = NULL;
    }
    COORD_SHOW(btn->click);
    COORD_HIDE(btn->active);
    rdman_coord_changed(en->rdman,en->button->root_coord);
    rdman_redraw_changed(en->rdman);

    btn->progm = progm = mb_progm_new(1, en->rdman);
    MB_TIMEVAL_SET(&start, 0, 500000);
    MB_TIMEVAL_SET(&playing, 0, 0);
    word = mb_progm_next_word(progm, &start, &playing);
    mb_visibility_new(VIS_HIDDEN, btn->click, word);
    mb_visibility_new(VIS_VISIBLE, btn->active, word);
    mb_progm_free_completed(progm);
    get_now(&now);
    mb_progm_start(progm, X_MB_tman(en->rt), &now);
}
mb_button_t *mb_button_new(engine_t *en,mb_sprite_t *sp, char *name)
{
    mb_button_t *btn = (mb_button_t *) malloc(sizeof(mb_button_t));
    char *buf = (char *) malloc(strlen(name)+5);

    btn->root = (coord_t *) MB_SPRITE_GET_OBJ(sp, name);
    printf("btn->root=%x\n",btn->root);
    sprintf(buf, "%s_normal", name);
    btn->normal = (coord_t *) MB_SPRITE_GET_OBJ(sp, buf);
    if (btn->normal == NULL) {
    	printf("Missing normal button, this is not a correct button\n");
    }
    sprintf(buf, "%s_active", name);
    btn->active = (coord_t *) MB_SPRITE_GET_OBJ(sp, buf);
    if (btn->active == NULL) {
    	printf("Missing active button, this is not a correct button\n");
    }
    sprintf(buf, "%s_click", name);
    btn->click = (coord_t *) MB_SPRITE_GET_OBJ(sp, buf);
    if (btn->click == NULL) {
    	printf("Missing click button, this is not a correct button\n");
    }
    btn->press = NULL;
    // Show only the normal button
    COORD_HIDE(btn->active);
    COORD_HIDE(btn->click);
    COORD_SHOW(btn->normal);
    // Move to the same position
    btn->active->matrix[2] = 200;
    btn->active->matrix[5] = 200;
    btn->normal->matrix[2] = 200;
    btn->normal->matrix[5] = 200;
    btn->click->matrix[2] = 200;
    btn->click->matrix[5] = 200;
    btn->en = en;
    printf("btn->root=%x\n",CMOUSE(btn->root));
    btn->obs_move = subject_add_event_observer(CMOUSE(btn->root), EVT_MOUSE_MOVE, mb_button_move,btn);
    btn->obs_press = subject_add_event_observer(CMOUSE(btn->root), EVT_MOUSE_BUT_PRESS, mb_button_pressed,btn);
    btn->obs_out = subject_add_event_observer(CMOUSE(btn->root), EVT_MOUSE_OUT, mb_button_out,btn);
    btn->progm = NULL;
    rdman_redraw_changed(en->rdman);
    return btn;
}


void mb_button_add_onClick(mb_button_t *b, void (*h)(void *arg), void *arg)
{
    b->press = h;
    b->arg = arg;
}

engine_t *engine_init()
{

    X_MB_runtime_t *rt;
    rt = X_MB_new(":0.0", 800, 600);
    engine_t *en = (engine_t *) malloc(sizeof(engine_t));

    en->rt = rt;
    en->rdman =  X_MB_rdman(rt);
    return en;
}

void engine_mainloop(engine_t *en)
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


void test(void *a)
{
    printf("Button is pressed.....\n");
}


int main(int argc, char * const argv[]) {
    subject_t *subject;
    engine_t *en;
    mb_button_t *b;

    en = engine_init();
    en->menu = menu_new(en->rdman, en->rdman->root_coord);
    en->button = button_new(en->rdman, en->rdman->root_coord);
    b = mb_button_new(en, (mb_sprite_t *) en->button, "btn");
    mb_button_add_onClick(b, test,NULL);

    en->rx = 0;
    en->ry = 0;

    /*
     * Register observers to subjects of events for objects.
     */
    subject = coord_get_mouse_event(en->menu->rect);
    subject_add_event_observer(subject,  EVT_MOUSE_BUT_RELEASE, add_rect, en);


    engine_mainloop(en);

    return 0;
}

/* vim: set ts=4 */
