#include <stdio.h>
#include <mb.h>
#include <string.h>
//#include "menu.h"
#include "mbapp.h"
#include "animated_menu.h"
#include "app.h"


char *menus[] = {
	"This is item 1",
	"This is item 2",
	"This is item 3",
	"This is item 4",
	"This is item 5",
	"This is item 6",
	"This is item 7",
	"This is item 8"
};


MBApp *myApp;

_MyApp_InitContent(int argc, char *argv[])
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    subject_t *key = MBAPP_keySubject(myApp);
    char name[255];
    coord_t *l;
    int i;
    mb_sprite_t *sprite=myApp->rootsprite;
    
    data->m = mb_animated_menu_new(myApp,myApp->rootsprite,"item",menus);
    mb_animated_menu_set_callback(data->m, myselect_callback);
    MyApp_InitContent(myApp,argc,argv);
}

int main(int argc, char * const argv[]) {
    subject_t *subject;
    mb_obj_t *button;
    MyAppData data;

    myApp = MBApp_Init("list");
    MBApp_setData(myApp,&data);
    _MyApp_InitContent(argc,argv);

    MBApp_loop(myApp);

    return 0;
}

/* vim: set ts=4 */
