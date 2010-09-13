// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
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

/*! \defgroup xnjsmb_observer JS binding for observer, subject and events
 * \ingroup xnjsmb
 *
 * @{
 */

struct xnjsmb_observer_data {
    Persistent<Function> func;
    Persistent<Context> ctx;
};

static void
event_handler(event_t *evt, void *arg);

static void
xnjsmb_event_mod(Handle<Object> self, event_t *evt) {
    mouse_event_t *mevt;
    X_kb_event_t *xkbevt;

    switch(evt->type) {
    case EVT_ANY:
    case EVT_MOUSE_OVER:
    case EVT_MOUSE_OUT:
    case EVT_MOUSE_MOVE:
    case EVT_MOUSE_BUT_PRESS:
    case EVT_MOUSE_BUT_RELEASE:
	mevt = (mouse_event_t *)evt;
	SET(self, "x", Integer::New(mevt->x));
	SET(self, "y", Integer::New(mevt->y));
	SET(self, "but_state", Integer::New(mevt->but_state));
	SET(self, "button", Integer::New(mevt->button));
	break;

    case EVT_KB_PRESS:
    case EVT_KB_RELEASE:
	xkbevt = (X_kb_event_t *)evt;
	SET(self, "keycode", Integer::New(xkbevt->keycode));
	SET(self, "sym", Integer::New(xkbevt->sym));
	break;

    case EVT_PROGM_COMPLETE:
    case EVT_RDMAN_REDRAW:
    case EVT_MONITOR_ADD:
    case EVT_MONITOR_REMOVE:
    case EVT_MONITOR_FREE:
    case EVT_MOUSE_MOVE_RAW:
    default:
	/* Not implemented.  Do nothing. */
	break;
    }
}

static observer_t *
_subject_add_event_observer(subject_t *subject, int type,
                            Handle<Function> func) {
    observer_t *observer;
    xnjsmb_observer_data *data;
    Handle<Context> ctx;

    data = new xnjsmb_observer_data;
    if(data == NULL)
	return NULL;
    data->func = Persistent<Function>::New(func);
    ctx = Context::GetCurrent();
    data->ctx = Persistent<Context>::New(ctx);
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

static Handle<Value>
xnjsmb_event_tgt_getter(Handle<Object> self, event_t *evt, const char **err) {
    Persistent<Object> *hdl;

    hdl = (Persistent<Object> *)
	mb_prop_get(&((mb_obj_t *)evt->tgt->obj)->props,
		    PROP_JSOBJ);
    return Local<Object>::New(*hdl);
}

static void
xnjsmb_event_tgt_setter(Handle<Object> self, event_t *evt,
			Handle<Value> value, const char **err) {
    *err = "Not implemented";
}

static Handle<Value>
xnjsmb_event_cur_tgt_getter(Handle<Object> self, event_t *evt,
			    const char **err) {
    Persistent<Object> *hdl;

    hdl = (Persistent<Object> *)
	mb_prop_get(&((mb_obj_t *)evt->cur_tgt->obj)->props,
		    PROP_JSOBJ);
    return Local<Object>::New(*hdl);
}

static void
xnjsmb_event_cur_tgt_setter(Handle<Object> self, event_t *evt,
			    Handle<Value> value, const char **err) {
    *err = "Not implemented";
}

/* This is the part of the code generated by gen_v8_binding.m4 */
#include "observer-inc.h"

static void
event_handler(event_t *evt, void *arg) {
    xnjsmb_observer_data *data = (xnjsmb_observer_data *)arg;
    Handle<Value> evt_obj;
    Handle<Value> func_args[1];
    Handle<Value> r;
    Context::Scope context_scope(data->ctx);
    TryCatch trycatch;

    evt_obj = xnjsmb_auto_event_new(evt);
    ASSERT(!evt_obj.IsEmpty());
    func_args[0] = evt_obj;
    r = data->func->Call(data->ctx->Global(), 1, func_args);
    if(r.IsEmpty()) {
	Handle<Value> exception = trycatch.Exception();
	String::AsciiValue exc_str(exception);
	fprintf(stderr, "Exception: %s\n", *exc_str);
    }
}

Handle<Value>
export_xnjsmb_auto_subject_new(subject_t *subject) {
    Handle<Value> val;

    val = xnjsmb_auto_subject_new(subject);
    return val;
}

void
xnjsmb_observer_init(void) {
    xnjsmb_auto_observer_init();
    xnjsmb_auto_subject_init();
    xnjsmb_auto_event_init();
}

/* @} */
