#ifndef __MB_BACKEND_H_
#define __MB_BACKEND_H_

#include "mb_redraw_man.h"
#include "mb_timer.h"
#include "mb_observer.h"
#include "mb_img_ldr.h"

#include "mb_config.h"

#ifdef X_BACKEND
#include "mb_X_supp.h"
#endif

#ifdef DFB_BACKEND
#inclde "mb_dfb_supp.h"
#endif

typedef void mb_rt_t;

typedef struct _mb_timer_man mb_timer_man_t;
typedef struct _mb_timer_factory mb_timer_factory_t;
typedef struct _mb_IO_man mb_IO_man_t;
typedef struct _mb_IO_factory mb_IO_factory_t;
typedef enum _MB_IO_TYPE MB_IO_TYPE;

/*! \brief Function signature of callback functions for IO requests.
 */
typedef void (*mb_IO_cb_t)(int hdl, int fd, MB_IO_TYPE type, void *data);

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
    mb_rt_t *(*new_with_win)(MB_DISPLAY display, MB_WINDOW win);
    
    void (*free)(mb_rt_t *rt);
    void (*free_keep_win)(mb_rt_t *rt);
    /*! \brief Request the backend to start monitoring a file descriptor.
     *
     * This is used only when the backend is responsible for event loop.
     */
    int (*add_event)(mb_rt_t *rt, int fd, MB_IO_TYPE type,
		     mb_IO_cb_t f,void *arg);
    /*! \brief Request the backend to stop monitoring a file descriptor.
     *
     * This is used only when the backend is responsible for event loop.
     */
    void (*remove_event)(mb_rt_t *rt, int hdl);
    /*! \brief Event Loop
     *
     * This is called when main application does not handle event
     * loop.  Or, it should register an IO factory (i.e \ref
     * mb_IO_factory_t) with the backend.
     */
    void (*event_loop)(mb_rt_t *rt);

    /*! \brief Flush requests to screen server if existed */
    int (*flush)(mb_rt_t *rt);
    
    subject_t *(*kbevents)(mb_rt_t *rt);
    redraw_man_t *(*rdman)(mb_rt_t *rt);
    mb_timer_man_t *(*timer_man)(mb_rt_t *rt);
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
enum _MB_IO_TYPE {MB_IO_DUMMY, MB_IO_R, MB_IO_W, MB_IO_RW};

/*! \brief IO Manager
 */
struct _mb_IO_man {
    int (*reg)(struct _mb_IO_man *io_man,
	       int fd, MB_IO_TYPE type, mb_IO_cb_t cb, void *data);
    void (*unreg)(struct _mb_IO_man *io_man,
		  int io_hdl);
};

/*! \brief Factory of IO managers.
 */
struct _mb_IO_factory {
    mb_IO_man_t *(*new)(void);
    void (*free)(mb_IO_man_t *io_man);
};

/*! \brief Function signature of callback functions for timers.
 */
typedef void (*mb_timer_cb_t)(int hdl,
			      const mb_timeval_t *tmo,
			      const mb_timeval_t *now,
			      void *data);

/*! \brief Timer manager
 */
struct _mb_timer_man {
    int (*timeout)(struct _mb_timer_man *tm_man,
		   mb_timeval_t *tmout, /* tiemout (wall time) */
		   mb_timer_cb_t cb, void *data);
    /*! \brief Remove a timeout request.
     *
     * \param tm_hdl is the handle returned by _mb_timer_man::timeout.
     */
    void (*remove)(struct _mb_timer_man *tm_man, int tm_hdl);
};

/*! \brief Factory of timer manager.
 */
struct _mb_timer_factory {
    mb_timer_man_t *(*new)(void);
    void (*free)(mb_timer_man_t *timer_man);
};

#endif /* __MB_BACKEND_H_ */
