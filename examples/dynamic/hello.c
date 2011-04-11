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
#include "mb_af.h"


mbaf_t *app;

typedef struct {
    shape_t *rect;
    co_aix orx,ory;
    int start_x,start_y;
    observer_t *obs1,*obs2;
    int currentscene;
}app_data_t;


void switch_scene(const mb_timeval_t *tmo, const mb_timeval_t *now,void *arg)
{
    app_data_t *en = MBAF_DATA((mbaf_t *)arg,app_data_t );
    mb_timeval_t timer,interval;

    
    get_now(&timer);
    MB_TIMEVAL_SET(&interval, 1 ,0);
    MB_TIMEVAL_ADD(&timer, &interval);
    mb_tman_timeout( mbaf_get_timer(app), &timer, switch_scene, app);

    en->currentscene = (en->currentscene + 1) % 2;
    printf("switch to scene %d\n", en->currentscene + 1);
    MB_SPRITE_GOTO_SCENE(app->rootsprite,en->currentscene + 1);
}

int main(int argc, char * const argv[]) {
    subject_t *subject;
    mb_button_t *b;
    mb_obj_t *button;
    app_data_t data;
    mb_timeval_t tmo,interval;

    if (argc > 1)
	app = mbaf_init(argv[1], "");
    else
	app = mbaf_init("scene", ".libs");
    data.currentscene=0;
    mbaf_set_data(app,&data);
    //b = mb_button_new(app, app->rootsprite, "btn");
    //mb_button_add_onClick(b, test,NULL);
    get_now(&tmo);
    MB_TIMEVAL_SET(&interval, 1 ,0);
    mb_tman_timeout( mbaf_get_timer(app), &tmo, switch_scene, app);
    

    mbaf_loop(app);

    return 0;
}

/* vim: set ts=4 */
