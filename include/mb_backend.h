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

struct _mb_rt;
typedef struct _mb_rt mb_rt_t;

struct _mb_timer_man;
struct _mb_timer_factory;
struct _mb_IO_man;
struct _mb_IO_factory;

/*! \brief Type of IO that registered with an IO manager.
 */
enum _MB_IO_TYPE {MB_IO_DUMMY, MB_IO_R, MB_IO_W, MB_IO_RW};

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
    mb_rt_t *(*rt_new)(const char *display, int w,int h);
    mb_rt_t *(*rt_new_with_win)(MB_DISPLAY display, MB_WINDOW win);
    
    void (*rt_free)(mb_rt_t *rt);
    void (*rt_free_keep_win)(mb_rt_t *rt);
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
    observer_factory_t *(*observer_factory)(mb_rt_t *rt);
    mb_img_ldr_t *(*loader)(mb_rt_t *rt);
    
    /*
     * Following two methods are used to integrate a backend to
     * event loop of main application.
     */
    void (*reg_IO_factory)(mb_IO_factory_t *io_man);
    void (*reg_timer_factory)(mb_timer_factory_t *tm_man);
} mb_backend_t;

#define mb_runtime_new(disp, w, h)	\
    mb_dfl_backend.rt_new((disp), (w), (h))
#define mb_runtime_new_with_win(disp, win)	\
    mb_dfl_backend.rt_new_with_win((disp), (win))
#define mb_reg_IO_factory(io_fact)	\
    mb_dfl_backend.reg_IO_factory(io_fact)
#define mb_reg_timer_factory(tm_fact)	\
    mb_dfl_backend.reg_timer_factory(tm_fact)

/*
 * This is defined by backend implementations.  For example, X_supp.c
 * or dfb_supp.c should defined a backend.
 */
extern mb_backend_t mb_dfl_backend;

#define mb_runtime_free(rt)			\
    mb_dfl_backend.rt_free(rt)
#define mb_runtime_free_keep_win(rt)		\
    mb_dfl_backend.rt_free_keep_win(rt)
#define mb_runtime_add_event(rt, fd, type, cb, arg)		\
    mb_dfl_backend.add_event((rt), (fd), (type), (cb), (arg))
#define mb_runtime_remove_event(hdl)		\
    mb_dfl_backend.remove_event((rt), (hdl))
#define mb_runtime_event_loop(rt)		\
    mb_dfl_backend.event_loop(rt)
#define mb_runtime_flush(rt)			\
    mb_dfl_backend.flush(rt)
#define mb_runtime_kbevents(rt)			\
    mb_dfl_backend.kbevents(rt)
#define mb_runtime_rdman(rt)			\
    mb_dfl_backend.rdman(rt)
#define mb_runtime_timer_man(rt)		\
    mb_dfl_backend.timer_man(rt)
#define mb_runtime_observer_factory(rt)		\
    mb_dfl_backend.observer_factory(rt)
#define mb_runtime_loader(rt)			\
    mb_dfl_backend.loader(rt)


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
    mb_IO_man_t *(*io_man_new)(void);
    void (*io_man_free)(mb_IO_man_t *io_man);
};

#define mb_io_man_reg(io_man, fd, type, cb, data)	\
    (io_man)->reg(io_man, fd, type, cb, data)
#define mb_io_man_unreg(io_man, io_hdl)		\
    (io_man)->unreg(io_man, io_hdl)
#define mb_io_man_new(io_fact) (io_fact)->io_man_new()
#define mb_io_man_free(io_fact, io_man) (io_fact)->io_man_free(io_man)

/*! \brief Function signature of callback functions for timers.
 */
typedef void (*mb_timer_cb_t)(int hdl,
			      const mb_timeval_t *tmo,
			      const mb_timeval_t *now,
			      void *data);

/*! \brief Timer manager
 */
struct _mb_timer_man {
    /*! \brief Setup a timeout callback.
     *
     * \return -1 for error.
     */
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
    mb_timer_man_t *(*timer_man_new)(void);
    void (*timer_man_free)(mb_timer_man_t *timer_man);
};

#define mb_timer_man_timeout(tm_man, tmout, cb, data)	\
    (tm_man)->timeout((tm_man), (tmout), (cb), (data))
#define mb_timer_man_remove(tm_man, tm_hdl)	\
    (tm_man)->remove((tm_man), (tm_hdl))
#define mb_timer_man_new(tm_fact) (tm_fact)->timer_man_new()
#define mb_timer_man_free(tm_fact, tm_man) (tm_fact)->timer_man_free(tm_man)

#endif /* __MB_BACKEND_H_ */
