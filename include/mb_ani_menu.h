#ifndef __ANIMATED_MENU_H
#define __ANIMATED_MENU_H
typedef struct _mb_animated_menu {
	char **titles;
	int *menus_y;
	int *items;
	int top;
	int cur;
	int max;
	int ready;
	int speed;
	mbaf_t *app;
	mb_sprite_t *sprite;
	mb_obj_t **objects;
	mb_obj_t *lightbar;
	void (*callback)(struct _mb_animated_menu *m, int sel);
	void (*update_callback)(struct _mb_animated_menu *m, int sel);
	mb_progm_t *progm;
	X_kb_event_t pending_keys[16];
    int pending_pos, pending_last;
} mb_animated_menu_t;
/** \brief Create an instace of animated menu. 
 *
 *   The objectnames is used to extract symbols from the SVG file. 
 *         ${objectnames}0 - ${objectnames}8 is the text object.
 *         ${objectnames}_lightbar is the lightbar.
 *
 */
mb_animated_menu_t *mb_animated_menu_new(mbaf_t *app,mb_sprite_t *sp,char *objnames,char *menus[]);
void mb_animated_menu_set_speed(mb_animated_menu_t *m,int speed);
int mb_animated_menu_get_speed(mb_animated_menu_t *m);
void mb_animated_menu_set_callback(mb_animated_menu_t *m, void (*f)(mb_animated_menu_t *m, int sel));
#endif
