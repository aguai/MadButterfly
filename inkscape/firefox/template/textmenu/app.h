#ifndef _APP_H
#define _APP_H
typedef struct {
	mb_animated_menu_t *m;
}MyAppData;

extern void MyApp_InitContent(mbaf_t *app,int argc, char *argv[]);

void myselect_callback(mb_animated_menu_t *m, int select);
#endif
