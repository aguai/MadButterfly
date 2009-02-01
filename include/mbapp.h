#ifndef __APP_H
#define __APP_H
typedef struct _mbapp MBApp;
struct _mbapp {
    void *rt;
    redraw_man_t *rdman;
    mb_sprite_t *rootsprite;
    mb_obj_t *root;
    subject_t *kbevents;
    void *private;
};
MBApp *MBApp_Init(char *module);
void MBApp_setData(MBApp *app,void *data);
mb_tman_t *MBApp_getTimer(MBApp *app);
void MBApp_loop(MBApp *en);
#define MBAPP_DATA(app,type) ((type *) ((app)->private))
#define MBAPP_RDMAN(app) (((MBApp *) app)->rdman)
#define MBAPP_keySubject(app) ((app)->kbevents)

#include "mbbutton.h"
#endif
