/*! \file
 *
 *  This is the demo program for the animated menu. We will use to test the MBAF API.
 */
#include <stdio.h>
#include <mb.h>
#include <string.h>
#include "menu.h"
#include "mbapp.h"

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
    int top;
    int cur;
    int max;
}MyAppData;

MBApp *myApp;

static void fillMenuContent()
{
    int i;
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    mb_sprite_t *sprite=myApp->rootsprite;
    shape_t *text;
    coord_t *group;
    coord_t *lightbar;
    char name[255];

    for(i=0;i<8;i++) {
	if (i+data->top > data->max) break;
        snprintf(name,sizeof(name),"item%dtext", i+1);
        text = (shape_t *) MB_SPRITE_GET_OBJ(sprite, name);
	if (text == NULL) {
		printf("Can not find object %s\n",name);
		continue;
	}
	sh_text_set_text(text,menus[i+data->top]);
        rdman_shape_changed(MBAPP_RDMAN(myApp), text);
    }
    for(;i<8;i++) {
        snprintf(name,sizeof(name),"item%dtext", i+1);
        text = (shape_t *) MB_SPRITE_GET_OBJ(sprite, name);
	if (text == NULL) {
		printf("Can not find object %s\n",name);
		continue;
	}
	sh_text_set_text(text,"");
        rdman_shape_changed(MBAPP_RDMAN(myApp), text);
    }
    lightbar = (coord_t *) MB_SPRITE_GET_OBJ(sprite, "lightbar");
    snprintf(name,sizeof(name),"item%d", data->cur+1);
    group = (coord_t *) MB_SPRITE_GET_OBJ(sprite, name);
    coord_x(lightbar) = coord_x(group);
    coord_y(lightbar) = coord_y(group);
    rdman_coord_changed(MBAPP_RDMAN(myApp), lightbar);
    rdman_redraw_changed(MBAPP_RDMAN(myApp));
}

void menu_up()
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);

    if (data->cur > 5)
	data->cur--;
    else {
        if (data->top > 0) {
	    data->top--;
        } else {
	    if (data->cur == 0) 
	        return;
	    data->cur--;
	}
    }
    fillMenuContent();
}
void menu_down()
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);

    if (data->cur < 5) {
	if (data->top+data->cur <= data->max)
	    data->cur++;
    } else  {
        if ((data->top+8) < data->max) {
	    data->top++;
        } else {
   	    if (data->cur+data->top < data->max-1) 
	        data->cur++;
	    else
	        return;
	}
	printf("top=%d\n",data->top);
    }
    fillMenuContent();
}
void menu_select()
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);

    printf("menu '%s' is selected\n", menus[data->top+data->cur]);
}

void keyHandler(event_t *ev, void *arg)
{
    X_kb_event_t *xkey = (X_kb_event_t *)ev;
    if(xkey->event.type != EVT_KB_PRESS) {
        return;
    }
    switch(xkey->sym) {
    case 0xff51:		/* left */
	break;

    case 0xff52:		/* up */
	menu_up();
	break;

    case 0xff53:		/* right */
	break;

    case 0xff54:		/* down */
	menu_down();
	break;

    case 0xff0d:		/* enter */
	menu_select();
	break;
    default:
	return;
    }
}

MyApp_InitContent()
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    subject_t *key = MBAPP_keySubject(myApp);
    
    data->top = 0;
    data->cur = 0;
    data->max = sizeof(menus)/sizeof(int)-1;

    fillMenuContent();
    subject_add_observer(key, keyHandler,NULL);
}

int main(int argc, char * const argv[]) {
    subject_t *subject;
    mb_obj_t *button;
    MyAppData data;
    mb_timeval_t tmo,interval;

    if (argc > 1) 
	    myApp = MBApp_Init(argv[1]);
    else
	    myApp = MBApp_Init("list");
    MBApp_setData(myApp,&data);
    MyApp_InitContent();

    MBApp_loop(myApp);

    return 0;
}

/* vim: set ts=4 */
