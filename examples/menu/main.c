/*! \file
 *
 *  This is the demo program for the animated menu. We will use to test the MBAF API.
 *  We need to have group item1-item9 in the SVG file. Initially, we will show
 *  item1-item8 only. When a up/down key is pressed, we will draw the next item in item9 and 
 *  add two words to move item1-item9 smoothly. The first word move items to the 3/4 position 
 *  fastly. The second will move it from 3/4 to the final position slowly to make retard effect.
 *
 *  If we press another key before the second words finish, we will delete the word and replace 
 *  it with a word to move it fastly to the final position and then we will repeat the procedure
 *  to add another two words to move it to the next position.
 */
#include <stdio.h>
#include <mb.h>
#include <string.h>
//#include "menu.h"
#include "mb_af.h"
#include "mb_ani_menu.h"



char *menus[] = {
	"Item 1",
	"Item 2",
	"Item 3",
	"Item 4",
	"Item 5",
	"Item 6",
	"Item 7",
	"Item 8",
	"Item 9",
	"Item 10",
	"Item 11",
	"Item 12",
	"Item 13",
	"Item 14",
	"Item 15",
	"Item 16",
	"Item 17",
	"Item 18",
};

typedef struct {
	mb_animated_menu_t *m;
}app_data_t;

mbaf_t *app;



void myselect(mb_animated_menu_t *m, int select)
{
    printf("menu %d is selected\n", select);
}


app_init_content()
{
    app_data_t *data = MBAF_DATA(app,app_data_t);
    subject_t *key = MBAF_KB_SUBJECT(app);
    char name[255];
    coord_t *l;
    int i;
    mb_sprite_t *sprite=app->rootsprite;
    
    data->m = mb_animated_menu_new(app,app->rootsprite,"item",menus);
    mb_animated_menu_set_callback(data->m, myselect);
}

int main(int argc, char * const argv[]) {
    subject_t *subject;
    mb_obj_t *button;
    app_data_t data;
    mb_timeval_t tmo,interval;

    if (argc > 1) 
	app = mbaf_init(argv[1], "");
    else
	app = mbaf_init("list", ".libs");
    mbaf_set_data(app,&data);
    app_init_content();

    mbaf_loop(app);

    return 0;
}

/* vim: set ts=4 */
