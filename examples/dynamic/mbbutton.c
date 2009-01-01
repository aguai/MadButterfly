
#include <stdio.h>
#include <mb.h>
#include <string.h>
#include "mbapp.h"



#define CMOUSE(e) (coord_get_mouse_event(e))
#define MBAPP_DATA(app,type) ((type *) ((app)->private))
#define MBAPP_RDMAN(app) (((MBApp *) app)->rdman)


static void mb_button_pressed(event_t *evt, void *arg);
static void mb_button_out(event_t *evt, void *arg);

void mb_button_add_onClick(mb_button_t *b, void (*h)(void *arg), void *arg)
{
    b->press = h;
    b->arg = arg;
}

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

static void mb_button_show_active(event_t *evt, void *arg)
{
    mb_button_t *btn = (mb_button_t *) arg;
    MBApp *en = btn->en;

    coord_show(btn->active);
    mb_button_refresh(btn);
}

static void mb_button_pressed(event_t *evt, void *arg)
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
