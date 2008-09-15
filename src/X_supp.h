#ifndef __X_SUPP_H_
#define __X_SUPP_H_

#include <X11/Xlib.h>
#include "mb_types.h"
#include "mb_timer.h"
#include "redraw_man.h"

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

extern void X_MB_handle_connection(X_MB_runtime_t *rt);
extern X_MB_runtime_t *X_MB_new(const char *display_name, int w, int h);
extern void X_MB_free(X_MB_runtime_t *xmb_rt);

extern subject_t *X_MB_kbevents(X_MB_runtime_t *xmb_rt);
extern redraw_man_t *X_MB_rdman(X_MB_runtime_t *xmb_rt);
extern mb_tman_t *X_MB_tman(X_MB_runtime_t *xmb_rt);

#endif
