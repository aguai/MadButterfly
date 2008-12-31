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


typedef struct _mbapp MBApp;
struct _mbapp {
    void *rt;
    redraw_man_t *rdman;
    mb_sprite_t *rootsprite;
    mb_obj_t *root;
    void *private;
};

typedef struct {
    shape_t *rect;
    co_aix orx,ory;
    int start_x,start_y;
    observer_t *obs1,*obs2;
    int currentscene;
}MyAppData;

#define MBAPP_DATA(app,type) ((type *) ((app)->private))
#define MBAPP_RDMAN(app) (((MBApp *) app)->rdman)


typedef struct _mb_button {
    mb_obj_t obj;
    MBApp *en;
    int state;
    coord_t *root;
    coord_t *active;
    coord_t *normal;
    coord_t *click;
    void (*press)();
    void *arg;
    observer_t *obs_move,*obs_out,*obs_press;
    mb_progm_t *progm;
} mb_button_t;



#define CMOUSE(e) (coord_get_mouse_event(e))


static void mb_button_pressed(event_t *evt, void *arg);
static void mb_button_out(event_t *evt, void *arg);

void mb_button_refresh(mb_button_t *btn)
{
    rdman_coord_changed(btn->en->rdman,btn->root);
    rdman_redraw_changed(btn->en->rdman);
}

static void mb_button_move(event_t *evt, void *arg) 
{
    mb_button_t *btn = (mb_button_t *) arg;
    MBApp *en = btn->en;

    
    printf("Mouse move\n");
    arg = (void *)en;
    coord_show(btn->active);
    mb_button_refresh(btn);
}
static void mb_button_out(event_t *evt, void *arg) 
{
    mb_button_t *btn = (mb_button_t *) arg;
    MBApp *en = btn->en;
    arg = (void *) en;

    if (btn->progm) {
	    mb_progm_abort(btn->progm);
	    btn->progm = NULL;
    }
    printf("mouse out\n");
    coord_hide(btn->click);
    coord_hide(btn->active);
    coord_show(btn->normal);
    mb_button_refresh(btn);
}

void mb_button_show_active(event_t *evt, void *arg)
{
    mb_button_t *btn = (mb_button_t *) arg;
    MBApp *en = btn->en;

    coord_show(btn->active);
    mb_button_refresh(btn);
}

void mb_button_pressed(event_t *evt, void *arg)
{
    mb_button_t *btn = (mb_button_t *) arg;
    MBApp *en = btn->en;
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;
    arg = (void *) en;

    printf("Pressed\n");
    if (btn->progm) {
	    mb_progm_abort(btn->progm);
	    btn->progm = NULL;
    }
    coord_show(btn->click);
    coord_hide(btn->active);
    rdman_coord_changed(MBAPP_RDMAN(arg),btn->root);
    rdman_redraw_changed(MBAPP_RDMAN(arg));

    btn->progm = progm = mb_progm_new(1, MBAPP_RDMAN(arg));
    MB_TIMEVAL_SET(&start, 0, 500000);
    MB_TIMEVAL_SET(&playing, 0, 0);
    word = mb_progm_next_word(progm, &start, &playing);
    mb_visibility_new(VIS_HIDDEN, btn->click, word);
    mb_visibility_new(VIS_VISIBLE, btn->active, word);
    mb_progm_free_completed(progm);
    get_now(&now);
    mb_progm_start(progm, X_MB_tman(en->rt), &now);
    if (btn->press)
    	btn->press(btn->arg);
}
mb_button_t *mb_button_new(MBApp *app,mb_sprite_t *sp, char *name)
{
    mb_button_t *btn = (mb_button_t *) malloc(sizeof(mb_button_t));
    char *buf = (char *) malloc(strlen(name)+5);
    MBApp *arg = app;

    btn->root = (coord_t *) MB_SPRITE_GET_OBJ(sp, name);
    sprintf(buf, "%s_normal", name);
    btn->normal = (coord_t *) MB_SPRITE_GET_OBJ(sp, buf);
    if (btn->normal == NULL) {
    	printf("Missing normal button, this is not a correct button\n");
    }
    sprintf(buf, "%s_active", name);
    btn->active = (coord_t *) MB_SPRITE_GET_OBJ(sp, buf);
    if (btn->active == NULL) {
    	printf("Missing click button, this is not a correct button\n");
    }
    sprintf(buf, "%s_click", name);
    btn->click = (coord_t *) MB_SPRITE_GET_OBJ(sp, buf);
    if (btn->active == NULL) {
    	printf("Missing click button, this is not a correct button\n");
    }
    btn->press = NULL;
    // Show only the normal button
    coord_hide(btn->active);
    coord_hide(btn->click);
    coord_show(btn->normal);
    // Move to the same position
    btn->active->matrix[2] = 200;
    btn->active->matrix[5] = 200;
    btn->normal->matrix[2] = 200;
    btn->normal->matrix[5] = 200;
    btn->click->matrix[2] = 200;
    btn->click->matrix[5] = 200;
    btn->en = app;
    btn->obs_move = subject_add_event_observer(CMOUSE(btn->root), EVT_MOUSE_MOVE, mb_button_move,btn);
    btn->obs_press = subject_add_event_observer(CMOUSE(btn->root), EVT_MOUSE_BUT_PRESS, mb_button_pressed,btn);
    btn->obs_out = subject_add_event_observer(CMOUSE(btn->root), EVT_MOUSE_OUT, mb_button_out,btn);
    btn->progm = NULL;
    rdman_redraw_changed(MBAPP_RDMAN(arg));
    return btn;
}


void mb_button_add_onClick(mb_button_t *b, void (*h)(void *arg), void *arg)
{
    b->press = h;
    b->arg = arg;
}

MBApp *MBApp_Init(char *module)
{
    MBApp *app = (MBApp *) malloc(sizeof(MBApp));
    X_MB_runtime_t *rt;

    rt = X_MB_new(":0.0", 800, 600);

    app->rt = rt;
    app->rdman =  X_MB_rdman(rt);
    app->rootsprite= sprite_load(module,app->rdman, app->rdman->root_coord);
    return app;
}

void MBApp_setData(MBApp *app,void *data)
{
    app->private = (void *) data;
}

mb_tman_t *MBApp_getTimer(MBApp *app)
{
    return X_MB_tman(app->rt);
}

void MBApp_loop(MBApp *en)
{
    /*
     * Start handle connections, includes one to X server.
     * User start to interact with the application.
     */
    X_MB_handle_connection(en->rt);

    /*
     * Clean
     */
    X_MB_free(en->rt);
    free(en);
}


static void add_rect_move(event_t *evt, void *arg) 
{
    MyAppData *en = MBAPP_DATA((MBApp *)arg,MyAppData );
    mouse_event_t *mev = (mouse_event_t *) evt;

    printf("resize rectangle\n");
    sh_rect_set(en->rect, en->start_x, en->start_y, mev->x - en->start_x, mev->y-en->start_y,0,0);
    rdman_shape_changed(MBAPP_RDMAN(arg),en->rect);
    rdman_redraw_changed(MBAPP_RDMAN(arg));
}

static void add_rect_release(event_t *evt, void *arg) 
{
    MyAppData *en = MBAPP_DATA((MBApp *)arg,MyAppData );
    mouse_event_t *mev = (mouse_event_t *) evt;

    printf("rectangle done\n");
    subject_remove_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), en->obs1);
    subject_remove_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), en->obs2);
}

static void add_rect_2(event_t *evt, void *arg) 
{
    MyAppData *en = MBAPP_DATA((MBApp *)arg,MyAppData );
    mouse_event_t *mev = (mouse_event_t *) evt;
    paint_t *color;

    printf("select first point\n");
    // Add an rect path 

    en->start_x = mev->x;
    en->start_y = mev->y;
    subject_remove_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), en->obs1);
    subject_remove_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), en->obs2);
    en->obs1 = subject_add_event_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), EVT_MOUSE_MOVE, add_rect_move, en);
    en->obs2 = subject_add_event_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), EVT_MOUSE_BUT_RELEASE, add_rect_release, en);
}

static void add_rect_2_move(event_t *evt, void *arg) 
{
    MyAppData *en = MBAPP_DATA((MBApp *)arg,MyAppData );
    mouse_event_t *mev = (mouse_event_t *) evt;

    sh_rect_set(en->rect, mev->x, mev->y, 50,50,0,0);
    rdman_shape_changed(MBAPP_RDMAN(arg),en->rect);
    rdman_redraw_changed(MBAPP_RDMAN(arg));
}

static void add_rect(event_t *evt, void *arg) 
{
    MyAppData *en = MBAPP_DATA((MBApp *)arg,MyAppData );
    mouse_event_t *mev = (mouse_event_t *) evt;
    paint_t *color;

    printf("menut selected\n");
    en->obs1 = subject_add_event_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), EVT_MOUSE_BUT_PRESS, add_rect_2, en);
    en->obs2 = subject_add_event_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), EVT_MOUSE_MOVE, add_rect_2_move, en);
    en->rect = rdman_shape_rect_new(MBAPP_RDMAN(arg), mev->x, mev->y, 50 , 50, 0,0);
    // Paint it with color
    en->obs1 = subject_add_event_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), EVT_MOUSE_BUT_PRESS, add_rect_2, en);
    en->obs2 = subject_add_event_observer(CMOUSE(MBAPP_RDMAN(arg)->root_coord), EVT_MOUSE_MOVE, add_rect_2_move, en);
    en->rect = rdman_shape_rect_new(MBAPP_RDMAN(arg), mev->x, mev->y, 50 , 50, 0,0);
    // Paint it with color
    color = rdman_paint_color_new(MBAPP_RDMAN(arg), 0.800000, 0.800000, 0.400000, 1.000000);
    rdman_paint_fill(MBAPP_RDMAN(arg), color, en->rect);
    // Add to the stage
    //rdman_add_shape(MBAPP_RDMAN(arg), en->rect, en->menu->root_coord);
}


void test(void *a)
{
    printf("Button is pressed.....\n");
}

MBApp *myApp;

void switch_scene(const mb_timeval_t *tmo, const mb_timeval_t *now,void *arg)
{
    MyAppData *en = MBAPP_DATA((MBApp *)arg,MyAppData );
    mb_timeval_t timer,interval;

    
    get_now(&timer);
    MB_TIMEVAL_SET(&interval, 1 ,0);
    MB_TIMEVAL_ADD(&timer, &interval);
    mb_tman_timeout( MBApp_getTimer(myApp), &timer, switch_scene, myApp);

    en->currentscene = (en->currentscene + 1) % 2;
    printf("switch to scene %d\n", en->currentscene + 1);
    MB_SPRITE_GOTO_SCENE(myApp->rootsprite,en->currentscene + 1);
}

int main(int argc, char * const argv[]) {
    subject_t *subject;
    mb_button_t *b;
    mb_obj_t *button;
    MyAppData data;
    mb_timeval_t tmo,interval;

    if (argc > 1) 
	    myApp = MBApp_Init(argv[1]);
    else
	    myApp = MBApp_Init("scene");
    data.currentscene=0;
    MBApp_setData(myApp,&data);
    //b = mb_button_new(myApp, myApp->rootsprite, "btn");
    //mb_button_add_onClick(b, test,NULL);
    get_now(&tmo);
    MB_TIMEVAL_SET(&interval, 1 ,0);
    mb_tman_timeout( MBApp_getTimer(myApp), &tmo, switch_scene, myApp);
    

    MBApp_loop(myApp);

    return 0;
}

/* vim: set ts=4 */
