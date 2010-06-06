#ifndef __X_SUPP_NJS_H_
#define __X_SUPP_NJS_H_

struct _njs_ev_data;
typedef struct _njs_ev_data njs_ev_data_t;

extern void X_njs_MB_handle_connection(njs_ev_data_t *ev_data);
extern void X_njs_MB_free(njs_ev_data_t *ev_data);
extern njs_ev_data_t *X_njs_MB_new(char *display_name, int w, int h);
extern void *_X_njs_MB_get_runtime(nsj_ev_data_t *ev_data);

#endif /* __X_SUPP_NJS_H_ */
