// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __SUPP_NJS_H_
#define __SUPP_NJS_H_

#include <ev.h>
#include <mb_backend.h>

typedef struct _njs_runtime {
    mb_rt_t *mb_rt;
} njs_runtime_t;

extern void njs_mb_reg_timer_man(void);
extern void njs_mb_reg_IO_man(void);
/* extern void njs_mb_init_handle_connection(njs_runtime_t *rt); */
extern void njs_mb_free(njs_runtime_t *rt);
extern njs_runtime_t *njs_mb_new(char *display_name, int w, int h);
extern void njs_mb_free_keep_win(njs_runtime_t *rt);
extern njs_runtime_t *njs_mb_new_with_win(void *display, long win);
extern int njs_mb_flush(njs_runtime_t *rt);
extern void njs_mb_handle_single_event(njs_runtime_t *rt, void *evt);
extern void njs_mb_no_more_event(njs_runtime_t *rt);
extern mb_rt_t *_njs_mb_get_runtime(njs_runtime_t *rt);

#define njs_mb_kbevents(rt) mb_runtime_kbevents((rt)->xrt)
#define njs_mb_rdman(rt) mb_runtime_rdman((rt)->xrt)
#define njs_mb_timer_man(rt) mb_runtime_timer_man((rt)->xrt)
#define njs_mb_ob_factory(rt) mb_runtime_ob_factory((rt)->xrt)
#define njs_mb_img_ldr(rt) mb_runtime_img_ldr((rt)->xrt)

#endif /* __SUPP_NJS_H_ */
