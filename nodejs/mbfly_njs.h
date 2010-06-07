#ifndef __MBFLY_NJS_H_
#define __MBFLY_NJS_H_

#include <v8.h>
extern "C" {
#include <mb.h>
}

#define THROW(x)						\
    do {							\
	v8::Handle<v8::Value> exc;				\
	exc = v8::Exception::Error(v8::String::New(x));		\
	return v8::ThrowException(exc);				\
    } while(0)
#define UNWRAP(o) v8::External::Unwrap((o)->GetInternalField(0))
#define WRAP(o, v) (o)->SetInternalField(0, v8::External::Wrap(v))
#define SET(o, n, v) (o)->Set(v8::String::New(n), v)
#define GET(o, n) (o)->Get(v8::String::New(n))

redraw_man_t *xnjsmb_rt_rdman(v8::Handle<v8::Object> mbrt);

#endif /* __MBFLY_NJS_H_ */
