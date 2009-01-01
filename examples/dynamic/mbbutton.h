#ifndef __MBBUTTON_H
#define __MBBUTTON_H
typedef struct _mb_button {
    mb_obj_t obj;
    MBApp *en;
    int state;
    coord_t *root;
    coord_t *active;
    coord_t *normal;
    coord_t *click;
    void (*press)();
    void *arg;
    observer_t *obs_move,*obs_out,*obs_press;
    mb_progm_t *progm;
} mb_button_t;
mb_button_t *mb_button_new(MBApp *app,mb_sprite_t *sp, char *name);
void mb_button_add_onClick(mb_button_t *b, void (*h)(void *arg), void *arg);
#endif

