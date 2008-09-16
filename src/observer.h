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
    int type;			/*!< event type (a.k.a. EVT_*  */
    subject_t *tgt, *cur_tgt;
};

/*! \brief Observer of observer pattern.
 *
 * A target for receiving events.
 */
struct _observer {
    evt_handler hdr;
    void *arg;
    observer_t *next;
};

/*! \brief Subject of observer pattern.
 *
 * Observer is a pattern to decouple caller and callee,
 * especial for multiple callee.
 * \see http://en.wikipedia.org/wiki/Observer_pattern
 */
struct _subject {
    int obj_type;		/*!< \brief type of object (a.k.a. OBJT_*). */
    void *obj;			/*!< \brief the object this subject for. */
    int flags;
    STAILQ(observer_t) observers;
};
/*! \brief Flag that make a subject to propagate events to parents. */
#define SUBF_STOP_PROPAGATE 0x1
#define SUBF_BUSY 0x2		/*!< \brief in subject_notify() */
#define SUBF_FREE 0x4		/*!< \brief in postponding subject_free() */

enum {OBJT_GEO, OBJT_COORD, OBJT_KB, OBJT_PROGM};

struct _mouse_event {
    event_t event;
    int x, y;
    unsigned int but_state;
    unsigned int button;
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
      EVT_MOUSE_BUT_PRESS, EVT_MOUSE_BUT_RELEASE,
      EVT_KB_PRESS, EVT_KB_RELEASE, EVT_PROGM_COMPLETE};

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
