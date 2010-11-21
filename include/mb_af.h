// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __APP_H
#define __APP_H

#include "mb_backend.h"

typedef struct _mbapp mbaf_t;
struct _mbapp {
    mb_rt_t *rt;
    redraw_man_t *rdman;
    mb_sprite_t *rootsprite;
    mb_obj_t *root;
    subject_t *kbevents;
    void *private;
};
mbaf_t *mbaf_init(const char *module, const char *module_dir);
void mbaf_set_data(mbaf_t *app,void *data);
mb_tman_t *mbaf_get_timer(mbaf_t *app);
void mbaf_loop(mbaf_t *app);
#define MBAF_DATA(app,type) ((type *) ((app)->private))
#define MBAF_RDMAN(app) (((mbaf_t *) app)->rdman)
#define MBAF_KB_SUBJECT(app) ((app)->kbevents)

#include "mbbutton.h"

#endif /* __MBAF_H_ */
