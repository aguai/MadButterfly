// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __DFB_SUPP_H_
#define __DFB_SUPP_H_

#include "mb_types.h"
#include "mb_timer.h"
#include "mb_redraw_man.h"
#include "mb_img_ldr.h"

/*! \ingroup xkb
 * @{
 */
typedef struct _X_kb_info X_kb_info_t;

struct _X_kb_event {
    event_t event;
    int keycode;
    int sym;
};
typedef struct _X_kb_event X_kb_event_t;

/* @} */

typedef struct _X_MB_runtime X_MB_runtime_t;

extern void X_MB_handle_connection(void *rt);
extern void *X_MB_new(const char *display_name, int w, int h);
extern void X_MB_free(void *xmb_rt);
extern void X_MB_free_keep_win(void *rt);

extern subject_t *X_MB_kbevents(void *xmb_rt);
extern redraw_man_t *X_MB_rdman(void *xmb_rt);
extern mb_tman_t *X_MB_tman(void *xmb_rt);
extern observer_factory_t *X_MB_observer_factory(void *xmb_rt);
extern mb_img_ldr_t *X_MB_img_ldr(void *xmb_rt);

#endif
