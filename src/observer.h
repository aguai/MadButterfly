#ifndef __OBSERVER_H_
#define __OBSERVER_H_

#include "tools.h"

typedef struct _event event_t;
typedef struct _observer observer_t;
typedef struct _subject subject_t;
typedef struct _mouse_event mouse_event_t;
typedef struct _ob_factory ob_factory_t;
typedef void (*evt_handler)(event_t *event, void *arg);

struct _event {
    int type;
    subject_t *tgt, *cur_tgt;
};

struct _observer {
    evt_handler hdr;
    void *arg;
    observer_t *next;
};

struct _subject {
    int obj_type;
    void *obj;
    int flags;
    STAILQ(observer_t) observers;
};
#define SUBF_STOP_PROPAGATE 0x1

enum {OBJT_GEO, OBJT_COORD};

struct _mouse_event {
    event_t event;
    int x, y;
    int button;
};

#define MOUSE_BUT1 0x1
#define MOUSE_BUT2 0x2
#define MOUSE_BUT3 0x4

/*! \brief Observer factory.
 *
 * It provides functions for allocation of subject and observer objects,
 * and strategy function for getting the subject of parent coord object.
 */
struct _ob_factory {
    subject_t *(*subject_alloc)(ob_factory_t *factory);
    void (*subject_free)(ob_factory_t *factory, subject_t *subject);
    observer_t *(*observer_alloc)(ob_factory_t *factory);
    void (*observer_free)(ob_factory_t *factory, observer_t *observer);
    /*! This is a strategy function to get subjects of parents. */
    subject_t *(*get_parent_subject)(ob_factory_t *factory,
				     subject_t *cur_subject);
};

enum {EVT_MOUSE_OVER, EVT_MOUSE_OUT, EVT_MOUSE_MOVE,
      EVT_MOUSE_BUT_PRESS, EVT_MOUSE_BUT_RELEASE};

extern subject_t *subject_new(ob_factory_t *factory,
			      void *obj, int obj_type);
extern void subject_free(ob_factory_t *factory, subject_t *subject);
extern void subject_notify(ob_factory_t *factory,
			   subject_t *subject, event_t *evt);
extern observer_t *subject_add_observer(ob_factory_t *factory,
					subject_t *subject,
					evt_handler hdr, void *arg);
extern void subject_remove_observer(ob_factory_t *factory,
				    subject_t *subject,
				    observer_t *observer);


#endif /* __OBSERVER_H_ */
