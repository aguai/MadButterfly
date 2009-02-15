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


typedef struct {
	char **titles;
	int *menus_y;
	int *items;
	int top;
	int cur;
	int max;
	int ready;
	MBApp *app;
	mb_sprite_t *sprite;
	mb_obj_t **objects;
	mb_obj_t *lightbar;
} mb_animated_menu_t;

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
	mb_animated_menu_t *m;
}MyAppData;

MBApp *myApp;


static void set_text(coord_t *g, char *text)
{
    geo_t *geo;
    shape_t *shape;

    FOR_COORD_MEMBERS(g, geo) {
        shape = geo_get_shape(geo);
        if(shape->obj.obj_type == MBO_TEXT) {
		sh_text_set_text(shape, text);
        }
    }
}

static void mb_animated_menu_fillMenuContent(mb_animated_menu_t *m)
{
    int i;
    coord_t *textgroup;
    shape_t *text;
    coord_t *group;
    coord_t *lightbar;
    int tmp;
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;


    // fill new item
    for(i=0;i<8;i++) {
        text = (shape_t *) m->objects[m->items[i]];
        set_text(text, m->titles[m->top+i]);
	//rdman_shape_changed(MBAPP_RDMAN(m->app),text);
    }


    textgroup = (coord_t *) m->objects[m->items[i]];
    coord_hide(textgroup);
    rdman_coord_changed(MBAPP_RDMAN(m->app),textgroup);

    lightbar = (coord_t *) m->lightbar;
    group = (coord_t *) m->objects[m->cur];
    coord_x(lightbar) = coord_x(group);
    coord_y(lightbar) = coord_y(group);
    rdman_redraw_changed(MBAPP_RDMAN(m->app));
}

static void mb_animated_menu_complete(void *arg)
{
    mb_animated_menu_t *m = (mb_animated_menu_t *) arg;

    m->ready++;
}

static void mb_animated_menu_fillMenuContentUp(mb_animated_menu_t *m)
{
    int i;
    coord_t *textgroup;
    shape_t *text;
    coord_t *group;
    coord_t *lightbar;
    int tmp;
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;


    // fill new item
    text = (shape_t *) m->objects[m->items[8]];
    set_text(text, m->titles[m->top]);

    progm = mb_progm_new(2, MBAPP_RDMAN(m->app));
    MB_TIMEVAL_SET(&start, 0, 0);
    MB_TIMEVAL_SET(&playing, 0, SPEED);
    word = mb_progm_next_word(progm, &start, &playing);
    get_now(&now);

    for(i=0;i<7;i++) {
	//shift to the next item
        textgroup = (coord_t *) m->objects[m->items[i]];
	mb_shift_new(0,m->menus_y[i+1]-coord_y(textgroup), textgroup,word);
    }
    // fade out the item[7]
    textgroup = (coord_t *) m->objects[m->items[7]];
    mb_shift_new(0,100, textgroup,word);

    // fade in the item[8]
    textgroup = (coord_t *) m->objects[m->items[8]];
    group = (coord_t *) m->objects[m->items[0]];
    coord_y(textgroup) = m->menus_y[0]-100;
    coord_show(textgroup);
    mb_shift_new(0,100, textgroup,word);

    lightbar = (coord_t *) m->lightbar;
    mb_shift_new(0,m->menus_y[m->cur]-coord_y(lightbar),lightbar,word);
    
    MB_TIMEVAL_SET(&start, 0, SPEED);
    MB_TIMEVAL_SET(&playing, 0, 0);
    word = mb_progm_next_word(progm, &start, &playing);
    textgroup = (coord_t *) m->objects[m->items[7]];
    mb_visibility_new(VIS_HIDDEN, textgroup,word);

    mb_progm_free_completed(progm);
    m->ready--;
    subject_add_observer(mb_progm_get_complete(progm), mb_animated_menu_complete,m);
    mb_progm_start(progm, X_MB_tman(MBAPP_RDMAN(m->app)->rt), &now);
    rdman_redraw_changed(MBAPP_RDMAN(m->app));
    tmp = m->items[8];
    for(i=8;i>0;i--) {
	m->items[i] = m->items[i-1];
    }
    m->items[0] = tmp;
}


static void mb_animated_menu_fillMenuContentDown(mb_animated_menu_t *m)
{
    int i;
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
    set_text(m->objects[m->items[8]], m->titles[m->top+7]);

    progm = mb_progm_new(2, MBAPP_RDMAN(m->app));
    MB_TIMEVAL_SET(&start, 0, 0);
    MB_TIMEVAL_SET(&playing, 0, SPEED);
    word = mb_progm_next_word(progm, &start, &playing);
    get_now(&now);

    for(i=1;i<8;i++) {
	//shift to the next item
	mb_shift_new(0,m->menus_y[i-1]-coord_y((coord_t *)m->objects[m->items[i]]), (coord_t *) m->objects[m->items[i]],word);
    }
    // fade out the item[0]
    mb_shift_new(0,-100, (coord_t *)m->objects[m->items[0]],word);

    // fade in the item[8]
    coord_y((coord_t *)m->objects[m->items[8]]) = m->menus_y[7]+100;
    coord_show(((coord_t *)(m->objects[m->items[8]])));
    mb_shift_new(0,-100, (coord_t *)m->objects[m->items[8]],word);

    mb_shift_new(0,m->menus_y[m->cur]-coord_y((coord_t *)m->lightbar),((coord_t *)(m->lightbar)),word);

    MB_TIMEVAL_SET(&start, 0, SPEED);
    MB_TIMEVAL_SET(&playing, 0, 0);
    word = mb_progm_next_word(progm, &start, &playing);
    mb_visibility_new(VIS_VISIBLE, (coord_t *) m->objects[m->items[0]],word);

    mb_progm_free_completed(progm);
    m->ready--;
    subject_add_observer(mb_progm_get_complete(progm), mb_animated_menu_complete,m);
    mb_progm_start(progm, X_MB_tman(MBAPP_RDMAN(m->app)->rt), &now);
    rdman_redraw_changed(MBAPP_RDMAN(m->app));
    tmp = m->items[0];
    for(i=0;i<8;i++) {
	m->items[i] = m->items[i+1];
    }
    m->items[8] = tmp;
}

void mb_animated_menu_moveLightBar(mb_animated_menu_t *m)
{
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;
    coord_t *group;
    coord_t *lightbar;

    progm = mb_progm_new(1, MBAPP_RDMAN(m->app));
    MB_TIMEVAL_SET(&start, 0, 0);
    MB_TIMEVAL_SET(&playing, 0, 200000);
    word = mb_progm_next_word(progm, &start, &playing);
    get_now(&now);

    lightbar = (coord_t *) m->lightbar;
    group = (coord_t *) m->objects[m->cur];
    mb_shift_new(coord_x(group)-coord_x(lightbar),coord_y(group)-coord_y(lightbar),lightbar,word);
    mb_progm_free_completed(progm);
    mb_progm_start(progm, X_MB_tman(MBAPP_RDMAN(m->app)->rt), &now);
    rdman_redraw_changed(MBAPP_RDMAN(m->app));
}

void mb_animated_menu_up(mb_animated_menu_t *m)
{
    if (m->cur > 5) {
	m->cur--;
        mb_animated_menu_moveLightBar(m);
    } else {
        if (m->top > 0) {
	    m->top--;
            mb_animated_menu_fillMenuContentUp(m);
        } else {
	    if (m->cur == 0) 
	        return;
	    m->cur--;
            mb_animated_menu_moveLightBar(m);
	}
    }
}
void mb_animated_menu_down(mb_animated_menu_t *m)
{

    if (m->cur < 4) {
	if (m->top+m->cur <= m->max) {
	    m->cur++;
            mb_animated_menu_moveLightBar(m);
	}
    } else  {
        if ((m->top+8) < m->max) {
	    m->top++;
            mb_animated_menu_fillMenuContentDown(m);
        } else {
   	    if (m->cur+m->top < m->max-1) {
	        m->cur++;
                mb_animated_menu_moveLightBar(m);
	    } else
	        return;
	}
    }
}
void mb_animated_menu_select(mb_animated_menu_t *m)
{
}

void mb_animated_menu_keyHandler(event_t *ev, void *arg)
{
    mb_animated_menu_t *m = (mb_animated_menu_t *) arg;
    X_kb_event_t *xkey = (X_kb_event_t *)ev;
    if(xkey->event.type != EVT_KB_PRESS) {
        return;
    }
    switch(xkey->sym) {
    case 0xff51:		/* left */
	break;

    case 0xff52:		/* up */
	mb_animated_menu_up(m);
	break;

    case 0xff53:		/* right */
	break;

    case 0xff54:		/* down */
	mb_animated_menu_down(m);
	break;

    case 0xff0d:		/* enter */
	mb_animated_menu_select(m);
	break;
    default:
	return;
    }
}

/** \brief Create an instace of animated menu. 
 *
 *   The objectnames is used to extract symbols from the SVG file. 
 *         ${objectnames}0 - ${objectnames}8 is the text object.
 *         ${objectnames}_lightbar is the lightbar.
 *
 */
mb_animated_menu_t *mb_animated_menu_new(MBApp *app,mb_sprite_t *sp,char *objnames,char *menus[])
{
    mb_animated_menu_t *m;
    int i,len;
    char name[255];
    mb_obj_t *l;

    for(i=0;menus[i];i++);
    len = i;
    
    m = (mb_animated_menu_t *) malloc(sizeof(mb_animated_menu_t)+sizeof(int)*len*2+sizeof(mb_obj_t *)*len);
    m->app = app;
    m->sprite = sp;
    m->top = 0;
    m->cur = 0;
    m->ready = 1;
    m->max = len;
    m->items = (int *) (m+1);
    m->menus_y = (int *) (m->items+len);
    m->objects = (mb_obj_t **) (m->menus_y+len);
    for(i=0;i<9;i++) {
        m->items[i] = i;
	snprintf(name,sizeof(name),"%s%d", objnames, i+1);
	l = MB_SPRITE_GET_OBJ(sp,name);
	if (l == NULL) {
		fprintf(stderr,"Can not find symbol %s\n",name);
	}
	m->objects[i] = (mb_obj_t *) l;
	m->menus_y[i] = coord_y((coord_t*)l);
    }
    m->titles = menus;
    snprintf(name,sizeof(name), "%s_lightbar", objnames);
    m->lightbar = (mb_obj_t *) MB_SPRITE_GET_OBJ(sp,name);
    if (m->lightbar)
	    fprintf(stderr,"Can not find object %s\n",name);
    mb_animated_menu_fillMenuContent(m);
    subject_add_observer(MBAPP_keySubject(myApp), mb_animated_menu_keyHandler,m);
    return m;
}

MyApp_InitContent()
{
    MyAppData *data = MBAPP_DATA(myApp,MyAppData);
    subject_t *key = MBAPP_keySubject(myApp);
    char name[255];
    coord_t *l;
    int i;
    mb_sprite_t *sprite=myApp->rootsprite;
    
    data->m = mb_animated_menu_new(myApp,myApp->rootsprite,"item",menus);
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
