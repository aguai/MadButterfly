#include <v8.h>
#include "mbfly_njs.h"

extern "C" {
#include <mb.h>
#include <string.h>
}

#ifndef ASSERT
#define ASSERT(x)
#endif

using namespace v8;

struct xnjsmb_observer_data {
    Persistent<Function> func;
};

static void
event_handler(event_t *evt, void *arg);

static observer_t *
_subject_add_event_observer(subject_t *subject, int type,
                            Handle<Function> func) {
    observer_t *observer;
    xnjsmb_observer_data *data;
    
    data = new xnjsmb_observer_data;
    if(data == NULL)
	return NULL;
    data->func = Persistent<Function>::New(func);
    observer = subject_add_event_observer(subject, type,
					  event_handler,
    	       				  data);

    return observer;
}

static void
_subject_remove_observer(subject_t *subject, observer_t *observer) {
    xnjsmb_observer_data *data;

    subject_remove_observer(subject, observer);
    data = (xnjsmb_observer_data *)observer->arg;
    delete data;
}

#include "observer-inc.h"

static void
event_handler(event_t *evt, void *arg) {
    xnjsmb_observer_data *data = (xnjsmb_observer_data *)arg;
    Handle<Value> evt_obj;
    Handle<Value> func_args[1];

    evt_obj = xnjsmb_event_new(evt);
    ASSERT(!evt_obj.IsEmpty());
    func_args[0] = evt_obj;
    data->func->Call(Context::GetCurrent()->Global(), 1, func_args);
}

