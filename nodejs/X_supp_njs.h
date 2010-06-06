#ifndef __X_SUPP_NJS_H_
#define __X_SUPP_NJS_H_

struct _njs_runtime;
typedef struct _njs_runtime njs_runtime_t;

extern void X_njs_MB_handle_connection(njs_runtime_t *rt);
extern void X_njs_MB_free(njs_runtime_t *rt);
extern njs_runtime_t *X_njs_MB_new(char *display_name, int w, int h);
extern void *_X_njs_MB_get_runtime(njs_runtime_t *rt);

#endif /* __X_SUPP_NJS_H_ */
