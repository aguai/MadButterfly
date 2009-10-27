#ifndef __X_SUPP_H_
#define __X_SUPP_H_

#include <X11/Xlib.h>
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

extern subject_t *X_MB_kbevents(void *xmb_rt);
extern redraw_man_t *X_MB_rdman(void *xmb_rt);
extern mb_tman_t *X_MB_tman(void *xmb_rt);
extern ob_factory_t *X_MB_ob_factory(void *xmb_rt);
extern mb_img_ldr_t *X_MB_img_ldr(void *xmb_rt);

#endif
