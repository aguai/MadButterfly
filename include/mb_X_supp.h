// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
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

typedef struct _X_supp_runtime X_supp_runtime_t;

typedef Window MB_WINDOW;
typedef Display *MB_DISPLAY;

#endif
