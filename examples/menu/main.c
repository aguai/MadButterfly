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

int menus_y[10];
int items[10];
#define SPEED 600000

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
    coord_t *textgroup;
    shape_t *text;
    coord_t *group;
    coord_t *lightbar;
    char name[255];
    int tmp;
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;


    // fill new item
    for(i=0;i<8;i++) {
        snprintf(name, sizeof(name),"item%dtext", items[i]);
        text = (shape_t *) MB_SPRITE_GET_OBJ(sprite,name);
        sh_text_set_text(text, menus[data->top+i]);
	rdman_shape_changed(MBAPP_RDMAN(myApp),text);
    }


    snprintf(name, sizeof(name),"item%d", items[i]);
    textgroup = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
    coord_hide(textgroup);
    rdman_coord_changed(MBAPP_RDMAN(myApp),textgroup);

    lightbar = (coord_t *) MB_SPRITE_GET_OBJ(sprite, "lightbar");
    snprintf(name,sizeof(name),"item%d", data->cur+1);
    group = (coord_t *) MB_SPRITE_GET_OBJ(sprite, name);
    coord_x(lightbar) = coord_x(group);
    coord_y(lightbar) = coord_y(group);
    rdman_redraw_changed(MBAPP_RDMAN(myApp));
}

static void fillMenuContentUp()
{
    int i;
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    mb_sprite_t *sprite=myApp->rootsprite;
    coord_t *textgroup;
    shape_t *text;
    coord_t *group;
    coord_t *lightbar;
    char name[255];
    int tmp;
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;


    // fill new item
    snprintf(name, sizeof(name),"item%dtext", items[8]);
    text = (shape_t *) MB_SPRITE_GET_OBJ(sprite,name);
    sh_text_set_text(text, menus[data->top]);

    progm = mb_progm_new(2, MBAPP_RDMAN(myApp));
    MB_TIMEVAL_SET(&start, 0, 0);
    MB_TIMEVAL_SET(&playing, 0, SPEED);
    word = mb_progm_next_word(progm, &start, &playing);
    get_now(&now);

    for(i=0;i<7;i++) {
	//shift to the next item
    	snprintf(name, sizeof(name),"item%d", items[i]);
        textgroup = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
	mb_shift_new(0,menus_y[i+1]-coord_y(textgroup), textgroup,word);
    }
    // fade out the item[7]
    snprintf(name, sizeof(name),"item%d", items[7]);
    textgroup = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
    mb_shift_new(0,100, textgroup,word);
    mb_visibility_new(VIS_HIDDEN, textgroup,word);

    // fade in the item[8]
    snprintf(name, sizeof(name),"item%d", items[8]);
    textgroup = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
    snprintf(name,sizeof(name),"item%d", items[0]);
    group = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
    coord_y(textgroup) = menus_y[0]-100;
    coord_show(textgroup);
    mb_shift_new(0,100, textgroup,word);

    lightbar = (coord_t *) MB_SPRITE_GET_OBJ(sprite, "lightbar");
    mb_shift_new(0,menus_y[data->cur]-coord_y(lightbar),lightbar,word);
    
    MB_TIMEVAL_SET(&start, 0, SPEED);
    MB_TIMEVAL_SET(&playing, 0, 0);
    word = mb_progm_next_word(progm, &start, &playing);
    snprintf(name, sizeof(name),"item%d", items[8]);
    textgroup = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
    mb_visibility_new(VIS_VISIBLE, textgroup,word);

    mb_progm_free_completed(progm);
    mb_progm_start(progm, X_MB_tman(MBAPP_RDMAN(myApp)->rt), &now);
    rdman_redraw_changed(MBAPP_RDMAN(myApp));
    tmp = items[8];
    for(i=8;i>0;i--) {
	items[i] = items[i-1];
    }
    items[0] = tmp;
}
static void fillMenuContentDown()
{
    int i;
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    mb_sprite_t *sprite=myApp->rootsprite;
    coord_t *textgroup;
    shape_t *text;
    coord_t *group;
    coord_t *lightbar;
    char name[255];
    int tmp;
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;


    // fill new item
    snprintf(name, sizeof(name),"item%dtext", items[8]);
    text = (shape_t *) MB_SPRITE_GET_OBJ(sprite,name);
    sh_text_set_text(text, menus[data->top+7]);

    progm = mb_progm_new(2, MBAPP_RDMAN(myApp));
    MB_TIMEVAL_SET(&start, 0, 0);
    MB_TIMEVAL_SET(&playing, 0, SPEED);
    word = mb_progm_next_word(progm, &start, &playing);
    get_now(&now);

    for(i=1;i<8;i++) {
	//shift to the next item
    	snprintf(name, sizeof(name),"item%d", items[i]);
        textgroup = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
	mb_shift_new(0,menus_y[i-1]-coord_y(textgroup), textgroup,word);
    }
    // fade out the item[0]
    snprintf(name, sizeof(name),"item%d", items[0]);
    textgroup = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
    mb_shift_new(0,-100, textgroup,word);

    // fade in the item[8]
    snprintf(name, sizeof(name),"item%d", items[8]);
    textgroup = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
    coord_y(textgroup) = menus_y[7]+100;
    coord_show(textgroup);
    mb_shift_new(0,-100, textgroup,word);

    lightbar = (coord_t *) MB_SPRITE_GET_OBJ(sprite, "lightbar");
    mb_shift_new(0,menus_y[data->cur]-coord_y(lightbar),lightbar,word);

    MB_TIMEVAL_SET(&start, 0, SPEED);
    MB_TIMEVAL_SET(&playing, 0, 0);
    word = mb_progm_next_word(progm, &start, &playing);
    snprintf(name, sizeof(name),"item%d", items[0]);
    textgroup = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
    mb_visibility_new(VIS_VISIBLE, textgroup,word);

    mb_progm_free_completed(progm);
    mb_progm_start(progm, X_MB_tman(MBAPP_RDMAN(myApp)->rt), &now);
    rdman_redraw_changed(MBAPP_RDMAN(myApp));
    tmp = items[0];
    for(i=0;i<8;i++) {
	items[i] = items[i+1];
    }
    items[8] = tmp;
}

MoveLightBar()
{
    mb_sprite_t *sprite=myApp->rootsprite;
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;
    coord_t *group;
    coord_t *lightbar;
    char name[255];

    progm = mb_progm_new(1, MBAPP_RDMAN(myApp));
    MB_TIMEVAL_SET(&start, 0, 0);
    MB_TIMEVAL_SET(&playing, 0, 200000);
    word = mb_progm_next_word(progm, &start, &playing);
    get_now(&now);

    lightbar = (coord_t *) MB_SPRITE_GET_OBJ(sprite, "lightbar");
    snprintf(name,sizeof(name),"item%d", items[data->cur]);
    group = (coord_t *) MB_SPRITE_GET_OBJ(sprite, name);
    mb_shift_new(coord_x(group)-coord_x(lightbar),coord_y(group)-coord_y(lightbar),lightbar,word);
    mb_progm_free_completed(progm);
    mb_progm_start(progm, X_MB_tman(MBAPP_RDMAN(myApp)->rt), &now);
    rdman_redraw_changed(MBAPP_RDMAN(myApp));
}

void menu_up()
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);

    if (data->cur > 5) {
	data->cur--;
        MoveLightBar();
    } else {
        if (data->top > 0) {
	    data->top--;
            fillMenuContentUp();
        } else {
	    if (data->cur == 0) 
	        return;
	    data->cur--;
            MoveLightBar();
	}
    }
}
void menu_down()
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);

    if (data->cur < 4) {
	if (data->top+data->cur <= data->max) {
	    data->cur++;
	    MoveLightBar();
	}
    } else  {
        if ((data->top+8) < data->max) {
	    data->top++;
            fillMenuContentDown();
        } else {
   	    if (data->cur+data->top < data->max-1) {
	        data->cur++;
	        MoveLightBar();
	    } else
	        return;
	}
    }
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
    char name[255];
    coord_t *l;
    int i;
    mb_sprite_t *sprite=myApp->rootsprite;
    
    data->top = 0;
    data->cur = 0;
    data->max = sizeof(menus)/sizeof(int)-1;
    for(i=0;i<9;i++) {
        items[i] = i+1;
	snprintf(name,255,"item%d", i+1);
	l = (coord_t *) MB_SPRITE_GET_OBJ(sprite,name);
	menus_y[i] = coord_y(l);
    }

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
