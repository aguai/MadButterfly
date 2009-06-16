/*! \file
 *
 * svg2code_ex is an example that show programmers how to create a
 * menu with MadButterfly.
 *
 */
#include <stdio.h>
#include <mb.h>
#include <string.h>
//#include "menu.h"
#include "mbapp.h"



typedef struct {
    shape_t *rect;
    co_aix orx,ory;
    int start_x,start_y;
    observer_t *obs1,*obs2;
    int currentscene;
}MyAppData;


#define CMOUSE(e) (coord_get_mouse_event(e))

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

MyApp_InitContent()
{
    mb_button_t *b;
    mb_sprite_t *sprite;

    sprite = sprite_load("button", myApp->rdman, myApp->rdman->root_coord);
    b = mb_button_new(myApp->rdman, sprite, "btn");
    mb_button_add_onClick(b, test,NULL);
}

void draw_text()
{
    
}

int main(int argc, char * const argv[]) {
    subject_t *subject;
    mb_obj_t *button;
    MyAppData data;
    mb_timeval_t tmo,interval;

    if (argc > 1) 
	    myApp = MBApp_Init(argv[1]);
    else
	    myApp = MBApp_Init("scene");
    data.currentscene=0;
    draw_text();
    MBApp_setData(myApp,&data);
    MyApp_InitContent();
    get_now(&tmo);
    MB_TIMEVAL_SET(&interval, 1 ,0);
    mb_tman_timeout( MBApp_getTimer(myApp), &tmo, switch_scene, myApp);
    

    MBApp_loop(myApp);

    return 0;
}

/* vim: set ts=4 */
