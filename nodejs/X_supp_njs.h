// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __X_SUPP_NJS_H_
#define __X_SUPP_NJS_H_

#include <ev.h>

typedef struct _njs_runtime {
    ev_io iowatcher;
    ev_timer tmwatcher;
    int enable_io;
    int enable_timer;
    void *xrt;
} njs_runtime_t;

extern void X_njs_MB_init_handle_connection(njs_runtime_t *rt);
extern void X_njs_MB_free(njs_runtime_t *rt);
extern njs_runtime_t *X_njs_MB_new(char *display_name, int w, int h);
extern void X_njs_MB_free_keep_win(njs_runtime_t *rt);
extern njs_runtime_t *X_njs_MB_new_with_win(void *display, long win);
extern int X_njs_MB_flush(njs_runtime_t *rt);
extern void X_njs_MB_handle_single_event(njs_runtime_t *rt, void *evt);
extern void X_njs_MB_no_more_event(njs_runtime_t *rt);
extern void *_X_njs_MB_get_X_runtime(njs_runtime_t *rt);

#define X_njs_MB_kbevents(rt) X_MB_kbevents((rt)->xrt)
#define X_njs_MB_rdman(rt) X_MB_rdman((rt)->xrt)
#define X_njs_MB_tman(rt) X_MB_tman((rt)->xrt)
#define X_njs_MB_ob_factory(rt) X_MB_ob_factory((rt)->xrt)
#define X_njs_MB_img_ldr(rt) X_MB_img_ldr((rt)->xrt)
#define X_njs_MB_kbevents(rt) X_MB_kbevents((rt)->xrt)

#endif /* __X_SUPP_NJS_H_ */
