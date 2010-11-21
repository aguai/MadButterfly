#ifndef __MB_BACKEND_H_
#define __MB_BACKEND_H_

#ifdef X_BACKEND
#include "mb_X_supp.h"
#endif

#ifdef DFB_BACKEND
#inclde "mb_dfb_supp.h"
#endif

typedef void *MBB_WINDOW;
typedef void mb_rt_t;

/*! \brief The backend engine mb_backend_t is used to define the
 *         interface to realize the MB.
 *
 * A backend is used to receive events from the system. The MB does
 * not define the backend by itself.  Instead, it define an interface
 * which allow the lower layer to implement the event system. Each
 * backend need to provides the following events.
 *
 * - keyboard event
 * - timer event
 * - image loader(?)
 * - render manager(?)
 */
typedef struct {
    mb_rt_t *(*new)(const char *display, int w,int h);
    mb_rt_t *(*new_with_win)(const char *display, MBB_WINDOW win, int w,int h);
    
    void (*free)(mb_rt_t *rt);
    void (*add_event)(mb_rt_t *rt,int type, int fd, mb_eventcb_t f,void *arg);
    void (*remove_event)(mb_rt_t *rt,int type, int fd);
    void (*loop)(mb_rt_t *rt);
    
    subject_t *(*kbevents)(mb_rt_t *rt);
    redraw_man_t *(*rdman)(mb_rt_t *rt);
    mb_timer_man_t *(*tman)(mb_rt_t *rt);
    ob_factory_t *(*ob_factory)(mb_rt_t *rt);
    mb_img_ldr_t *(*loader)(mb_rt_t *rt);
    
    /*
     * Following two methods are used to integrate a backend to
     * event loop of main application.
     */
    void (*reg_IO_factory)(mb_IO_factory_t *evman);
    void (*reg_timer_factory)(mb_timer_factory_t *evman);
} mb_backend_t;

extern mb_backend_t backend;

/*! \brief Type of IO that registered with an IO manager.
 */
enum MB_IO_TYPE {MB_IO_R, MB_IO_W, MB_IO_RW};

/*! \brief Function signature of callback functions for IO requests.
 */
typedef void (*mb_IO_cb_t)(int fd, MB_IO_TYPE type, void *data);

/*! \brief IO Manager
 */
struct _mb_IO_man {
    int (*reg)(struct _mb_IO_man *io_man,
	       int fd, MB_IO_TYPE type, mb_IO_cb_t cb, void *data);
    void (*unreg)(struct _mb_IO_Man *io_man,
		  int io_hdl);
};
typedef struct _mb_IO_man mb_IO_man_t;

/*! \brief Factory of IO managers.
 */
struct _mb_IO_factory {
    mb_IO_man_t *(*new)(void);
    void (*free)(mb_IO_man_t *io_man);
};
typedef struct _mb_IO_factory mb_IO_factory_t;

/*! \brief Function signature of callback functions for timers.
 */
typedef void (*mb_timer_cb_t)(mbsec_t sec, mbusec_t usec, void *data);

/*! \brief Timer manager
 */
struct _mb_timer_man {
    int (*timeout)(struct _mb_timer_man *tm_man,
		   mbsec_t sec, mbusec_t usec, mb_timer_cb_t cb, void *data);
    /*! \brief Remove a timeout request.
     *
     * \param tm_hdl is the handle returned by _mb_timer_man::timeout.
     */
    void (*remove)(struct _mb_timer_man *tm_man, int tm_hdl);
} mb_timer_man_t;

/*! \brief Factory of timer manager.
 */
struct _mb_timer_factory {
    mb_timer_man_t *(*new)(void);
    void (*free)(mb_timer_man_t *timer_man);
};
typedef struct _mb_timer_factory mb_timer_factory_t;

#endif /* __MB_BACKEND_H_ */
