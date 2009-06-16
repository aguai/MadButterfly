
#include <stdio.h>
#include <mb.h>
#include <string.h>
#include "mbapp.h"



#define CMOUSE(e) (coord_get_mouse_event(e))


static void mb_button_pressed(event_t *evt, void *arg);
static void mb_button_out(event_t *evt, void *arg);

void mb_button_add_onClick(mb_button_t *b, void (*h)(void *arg), void *arg)
{
    b->press = h;
    b->arg = arg;
}

void mb_button_refresh(mb_button_t *btn)
{
    rdman_coord_changed(btn->rdman,btn->root);
    rdman_redraw_changed(btn->rdman);
}

static void mb_button_move(event_t *evt, void *arg) 
{
    mb_button_t *btn = (mb_button_t *) arg;

    
    printf("Mouse move\n");
    coord_show(btn->active);
    mb_button_refresh(btn);
}
static void mb_button_out(event_t *evt, void *arg) 
{
    mb_button_t *btn = (mb_button_t *) arg;

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

    coord_show(btn->active);
    mb_button_refresh(btn);
}

static void mb_button_pressed(event_t *evt, void *arg)
{
    mb_button_t *btn = (mb_button_t *) arg;
    mb_timeval_t start, playing, now;
    mb_progm_t *progm;
    mb_word_t *word;

    printf("Pressed\n");
    if (btn->progm) {
	    mb_progm_abort(btn->progm);
	    btn->progm = NULL;
    }
    coord_show(btn->click);
    coord_hide(btn->active);
    rdman_coord_changed(btn->rdman,btn->root);
    rdman_redraw_changed(btn->rdman);

    btn->progm = progm = mb_progm_new(1, btn->rdman);
    MB_TIMEVAL_SET(&start, 0, 500000);
    MB_TIMEVAL_SET(&playing, 0, 0);
    word = mb_progm_next_word(progm, &start, &playing);
    mb_visibility_new(VIS_HIDDEN, btn->click, word);
    mb_visibility_new(VIS_VISIBLE, btn->active, word);
    mb_progm_free_completed(progm);
    get_now(&now);
    printf("rt = %x\n", btn->rdman->rt);
    mb_progm_start(progm, X_MB_tman(btn->rdman->rt), &now);
    if (btn->press)
    	btn->press(btn->arg);
}
mb_button_t *mb_button_new(redraw_man_t *rdman,mb_sprite_t *sp, char *name)
{
    mb_button_t *btn = (mb_button_t *) malloc(sizeof(mb_button_t));
    char *buf = (char *) malloc(strlen(name)+5);

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
    btn->rdman = rdman;
    btn->obs_move = subject_add_event_observer(CMOUSE(btn->root), EVT_MOUSE_MOVE, mb_button_move,btn);
    btn->obs_press = subject_add_event_observer(CMOUSE(btn->root), EVT_MOUSE_BUT_PRESS, mb_button_pressed,btn);
    btn->obs_out = subject_add_event_observer(CMOUSE(btn->root), EVT_MOUSE_OUT, mb_button_out,btn);
    btn->progm = NULL;
    rdman_redraw_changed(rdman);
    return btn;
}
