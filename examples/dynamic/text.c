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


MBApp *myApp;

typedef struct {
    shape_t *rect;
    co_aix orx,ory;
    int start_x,start_y;
    observer_t *obs1,*obs2;
    int currentscene;
}MyAppData;


void switch_scene(const mb_timeval_t *tmo, const mb_timeval_t *now,void *arg)
{
    MyAppData *en = MBAPP_DATA((MBApp *)arg,MyAppData );
    mb_timeval_t timer,interval;
    shape_t *text = (shape_t *) MB_SPRITE_GET_OBJ(myApp->rootsprite,"mytext");
    mb_textstyle_t style;

    mb_textstyle_init(&style);

    
    get_now(&timer);
    MB_TIMEVAL_SET(&interval, 1 ,0);
    MB_TIMEVAL_ADD(&timer, &interval);
    mb_tman_timeout( MBApp_getTimer(myApp), &timer, switch_scene, myApp);

    en->currentscene = (en->currentscene + 1) % 2;
    printf("xxx\n");
    if (en->currentscene == 0) {
        sh_text_set_text(text,"This is 0");
	mb_textstyle_set_color(&style, TEXTCOLOR_RGB(255,0,0));
	sh_text_set_style(text,0,5,&style);
    } else {
        sh_text_set_text(text,"This is 1");
	mb_textstyle_set_color(&style, TEXTCOLOR_RGB(0,255,0));
	sh_text_set_style(text,0,5,&style);
    }
    rdman_shape_changed(MBAPP_RDMAN(myApp), text);
#if 0
    /* Removed!
     * X_MB_handle_connection() will invoke it automatically.
     */
    rdman_redraw_changed(MBAPP_RDMAN(myApp));
#endif
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
	    myApp = MBApp_Init(".libs/mytext");
    data.currentscene=0;
    MBApp_setData(myApp,&data);
    get_now(&tmo);
    MB_TIMEVAL_SET(&interval, 1 ,0);
    mb_tman_timeout( MBApp_getTimer(myApp), &tmo, switch_scene, myApp);
    

    MBApp_loop(myApp);

    return 0;
}

/* vim: set ts=4 */
