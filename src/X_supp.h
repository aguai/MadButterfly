#ifndef __X_SUPP_H_
#define __X_SUPP_H_

#include <X11/Xlib.h>
#include "mb_timer.h"
#include "redraw_man.h"

typedef struct _X_MB_runtime X_MB_runtime_t;
struct _X_MB_runtime {
    Display *display;
    Window win;
    Visual *visual;
    cairo_surface_t *surface, *backend_surface;
    cairo_t *cr, *backend_cr;
    redraw_man_t *rdman;
    mb_tman_t *tman;
    int w, h;
};

extern void X_MB_handle_connection(Display *display,
				   redraw_man_t *rdman,
				   mb_tman_t *tman);
extern int X_MB_init(const char *display_name,
		     int w, int h, X_MB_runtime_t *xmb_rt);
extern void X_MB_destroy(X_MB_runtime_t *xmb_rt);


#endif
