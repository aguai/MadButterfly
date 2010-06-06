#include <stdio.h>
#include <v8.h>

extern "C" {
#include "mb.h"
#include "mb_X_supp.h"
#include "mb_tools.h"
#include "X_supp_njs.h"
}

using namespace v8;

static Handle<Value>
xnjsmb_coord_get_index(uint32_t index, const AccessorInfo &info) {
    HandleScope scope;
    Handle<Object> self;
    coord_t *coord;
    co_aix v;
    Handle<Value> exc;

    if(index < 0 || index >= 6) {
	exc = Exception::Error(String::New("Invalid index"));
	return ThrowException(exc);
    }
    
    self = info.This();
    coord = (coord_t *)External::Unwrap(self->Get(String::New("_njs_coord")));
    v = coord_get_matrix(coord)[index];

    return Number::New(v);
}

static Handle<Value>
xnjsmb_coord_set_index(uint32_t index, Local<Value> value,
		       const AccessorInfo &info) {
    
    HandleScope scope;
    Handle<Object> self;
    redraw_man_t *rdman;
    coord_t *coord;
    co_aix v;
    Handle<Value> exc;

    if(index < 0 || index >= 6) {
	exc = Exception::Error(String::New("Invalid index"));
	return ThrowException(exc);
    }
    if(!value->IsNumber()) {
	exc = Exception::Error(String::New("Invalid value"));
	return ThrowException(exc);
    }

    self = info.This();
    coord = (coord_t *)External::Unwrap(self->Get(String::New("_njs_coord")));
    v = value->NumberValue();
    coord_get_matrix(coord)[index] = v;

    rdman = (redraw_man_t *)
	External::Unwrap(self->Get(String::New("_njs_rdman")));
    rdman_coord_changed(rdman, coord);

    return value;
}
