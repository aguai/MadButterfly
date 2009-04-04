#include <stdio.h>
#include "mb_redraw_man.h"
#include "mb_observer.h"
#include "mb_tools.h"

#ifndef ASSERT
#define ASSERT(x)
#endif

subject_t *subject_new(ob_factory_t *factory, void *obj, int obj_type) {
    subject_t *subject;

    subject = factory->subject_alloc(factory);
    if(subject == NULL)
	return NULL;

    subject->obj = obj;
    subject->obj_type = obj_type;
    subject->flags = 0;
    subject->monitor_sub = NULL;
    STAILQ_INIT(subject->observers);

    subject->factory = factory;

    return subject;
}

/*!
 * \todo Keep ob_factory following subject objects.
 */
void subject_free(subject_t *subject) {
    ob_factory_t *factory = subject->factory;
    observer_t *observer;
    monitor_event_t mevt;

    ASSERT(!(subject->flags & SUBF_FREE));
    if(subject->flags & SUBF_BUSY) {
	/* Postpond the request until busy status been stoped.
	 * SUBF_BUSY means in subject_notify().
	 */
	subject->flags |= SUBF_FREE;
	return;
    }
    
    if(subject->monitor_sub) {
	mevt.event.type = EVT_MONITOR_FREE;
	mevt.subject = subject;
	mevt.observer = NULL;
	subject_notify(subject->monitor_sub, (event_t *)&mevt);
    }

    while((observer = STAILQ_HEAD(subject->observers))) {
	STAILQ_REMOVE(subject->observers, observer_t, next, observer);
	factory->observer_free(factory, observer);
    }
    factory->subject_free(factory, subject);
}


void subject_notify(subject_t *subject, event_t *evt) {
    ob_factory_t *factory = subject->factory;
    observer_t *observer;
    subject_t *old_subject;
    int stop_propagate = 0;
    int old_busy;

    evt->tgt = subject;
    evt->flags = 0;
    while(subject) {
	/*!
	 * \note What is happend when the subject is freed by observer?
	 *		Postponding the request of free until notification
	 *		been finished. (\ref SUBF_BUSY / \ref SUBF_FREE)
	 */
	old_busy = subject->flags & SUBF_BUSY;
	subject->flags |= SUBF_BUSY;

	evt->cur_tgt = subject;
	for(observer = STAILQ_HEAD(subject->observers);
	    observer != NULL;
	    observer = STAILQ_NEXT(observer_t, next, observer)) {
	    if (observer->type == EVT_ANY || observer->type == evt->type) {
		observer->hdr(evt, observer->arg);
		
		if(evt->flags & EVTF_STOP_NOTIFY) {
		    stop_propagate = 1;
			break;
		}
		if(evt->flags & EVTF_STOP_PROPAGATE)
		    stop_propagate = 1;
	    }
	}

	if(!old_busy)
	    subject->flags &= ~SUBF_BUSY;

	old_subject = subject;
	subject = factory->get_parent_subject(factory, subject);

	if(old_subject->flags & SUBF_STOP_PROPAGATE)
	    stop_propagate = 1;

	if(old_subject->flags & SUBF_FREE)
	    subject_free(old_subject);

	if(stop_propagate)
	    break;
    }
}

/*! \brief Add an observer for specified type of events.
 */
observer_t *subject_add_event_observer(subject_t *subject, int type,
				 evt_handler hdr, void *arg) {
    ob_factory_t *factory = subject->factory;
    observer_t *observer;
    monitor_event_t mevt;

    observer = factory->observer_alloc(factory);
    if(observer == NULL)
	return NULL;
    observer->hdr = hdr;
    observer->arg = arg;
    observer->type = type;

    STAILQ_INS_TAIL(subject->observers, observer_t, next, observer);

    if(subject->monitor_sub) {
	mevt.event.type = EVT_MONITOR_ADD;
	mevt.subject = subject;
	mevt.observer = observer;
	subject_notify(subject->monitor_sub, (event_t *)&mevt);
    }

    return observer;
}

/*! \brief Add an observer for specified type of events at head.
 */
observer_t *subject_add_event_observer_head(subject_t *subject, int type,
					    evt_handler hdr, void *arg) {
    ob_factory_t *factory = subject->factory;
    observer_t *observer;
    monitor_event_t mevt;

    observer = factory->observer_alloc(factory);
    if(observer == NULL)
	return NULL;
    observer->hdr = hdr;
    observer->arg = arg;
    observer->type = type;

    STAILQ_INS(subject->observers, observer_t, next, observer);

    if(subject->monitor_sub) {
	mevt.event.type = EVT_MONITOR_ADD;
	mevt.subject = subject;
	mevt.observer = observer;
	subject_notify(subject->monitor_sub, (event_t *)&mevt);
    }

    return observer;
}

void subject_remove_observer(subject_t *subject,
			     observer_t *observer) {
    ob_factory_t *factory = subject->factory;
    monitor_event_t mevt;

    STAILQ_REMOVE(subject->observers, observer_t, next, observer);
    
    if(subject->monitor_sub) {
	mevt.event.type = EVT_MONITOR_REMOVE;
	mevt.subject = subject;
	mevt.observer = observer;
	subject_notify(subject->monitor_sub, (event_t *)&mevt);
    }

    factory->observer_free(factory, observer);
}

#ifdef UNITTEST

#include <CUnit/Basic.h>
#include <stdlib.h>

static subject_t *test_subject_alloc(ob_factory_t *factory) {
    subject_t *subject;

    subject = (subject_t *)malloc(sizeof(subject_t));
    return subject;
}

static void test_subject_free(ob_factory_t *factory, subject_t *subject) {
    free(subject);
}

static observer_t *test_observer_alloc(ob_factory_t *factory) {
    observer_t *observer;

    observer = (observer_t *)malloc(sizeof(observer_t));
    return observer;
}

static void test_observer_free(ob_factory_t *factory, observer_t *observer) {
    free(observer);
}

static subject_t *test_get_parent_subject(ob_factory_t *factory,
					  subject_t *subject) {
    return NULL;
}

static ob_factory_t test_factory = {
    test_subject_alloc,
    test_subject_free,
    test_observer_alloc,
    test_observer_free,
    test_get_parent_subject
};

static void handler(event_t *evt, void *arg) {
    int *cnt = (int *)arg;

    CU_ASSERT(evt->type == EVT_MOUSE_OUT);
    (*cnt)++;
}

static void test_observer(void) {
    subject_t *subject;
    observer_t *observer[2];
    event_t evt;
    int cnt = 0;

    subject = subject_new(&test_factory, NULL, 0);
    subject->flags |= SUBF_STOP_PROPAGATE;
    observer[0] = subject_add_observer(subject, handler, &cnt);
    observer[1] = subject_add_observer(subject, handler, &cnt);

    evt.type = EVT_MOUSE_OUT;
    evt.tgt = NULL;
    evt.cur_tgt = NULL;
    subject_notify(subject, &evt);
    CU_ASSERT(cnt == 2);

    subject_remove_observer(subject, observer[0]);
    subject_remove_observer(subject, observer[1]);
    subject_free(subject);
}

static void monitor_handler(event_t *evt, void *arg) {
    int *cnt = (int *)arg;

    (*cnt)++;
}

static void test_monitor(void) {
    subject_t *subject, *monitor;
    observer_t *observer[2];
    int cnt = 0;

    subject = subject_new(&test_factory, NULL, 0);
    monitor = subject_new(&test_factory, NULL, 0);
    subject_set_monitor(subject, monitor);

    observer[0] = subject_add_observer(monitor, monitor_handler, &cnt);
    observer[1] = subject_add_observer(subject, NULL, NULL);
    CU_ASSERT(cnt == 1);

    subject_remove_observer(subject, observer[1]);
    CU_ASSERT(cnt == 2);

    subject_free(subject);
    CU_ASSERT(cnt == 3);

    subject_remove_observer(monitor, observer[0]);
    subject_free(monitor);
}

CU_pSuite get_observer_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_observer", NULL, NULL);
    CU_ADD_TEST(suite, test_observer);
    CU_ADD_TEST(suite, test_monitor);

    return suite;
}

#endif /* UNITTEST */
