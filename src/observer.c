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

    ASSERT(!(subject->flags & SUBF_FREE));
    if(subject->flags & SUBF_BUSY) {
	/* Postpond the request until busy status been stoped.
	 * SUBF_BUSY means in subject_notify().
	 */
	subject->flags |= SUBF_FREE;
	return;
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

    evt->tgt = subject;
    while(subject) {
	/*!
	 * \note What is happend when the subject is freed by observer?
	 *		Postponding the request of free until notification
	 *		been finished. (\ref SUBF_BUSY / \ref SUBF_FREE)
	 */
	subject->flags |= SUBF_BUSY;

	evt->cur_tgt = subject->obj;
	for(observer = STAILQ_HEAD(subject->observers);
	    observer != NULL;
	    observer = STAILQ_NEXT(observer_t, next, observer)) {
	    if (observer->type == EVT_ANY || observer->type == evt->type)
		    observer->hdr(evt, observer->arg);
	}

	subject->flags &= ~SUBF_BUSY;
	if(subject->flags & SUBF_FREE)
	    subject_free(subject);

	if(subject->flags & SUBF_STOP_PROPAGATE)
	    break;

	subject = factory->get_parent_subject(factory, subject);
    }

}

observer_t *subject_add_observer(subject_t *subject,
				 evt_handler hdr, void *arg) {
    ob_factory_t *factory = subject->factory;
    observer_t *observer;

    observer = factory->observer_alloc(factory);
    if(observer == NULL)
	return NULL;
    observer->hdr = hdr;
    observer->arg = arg;
    observer->type = EVT_ANY;

    STAILQ_INS_TAIL(subject->observers, observer_t, next, observer);

    return observer;
}

observer_t *subject_add_event_observer(subject_t *subject, int type,
				 evt_handler hdr, void *arg) {
    ob_factory_t *factory = subject->factory;
    observer_t *observer;

    observer = factory->observer_alloc(factory);
    if(observer == NULL)
	return NULL;
    observer->hdr = hdr;
    observer->arg = arg;
    observer->type = type;

    STAILQ_INS_TAIL(subject->observers, observer_t, next, observer);

    return observer;
}

void subject_remove_observer(subject_t *subject,
			     observer_t *observer) {
    ob_factory_t *factory = subject->factory;

    STAILQ_REMOVE(subject->observers, observer_t, next, observer);
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

void test_observer(void) {
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

CU_pSuite get_observer_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_observer", NULL, NULL);
    CU_ADD_TEST(suite, test_observer);

    return suite;
}

#endif /* UNITTEST */
