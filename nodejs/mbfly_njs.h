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

/* From coord.cc */
v8::Handle<v8::Value> xnjsmb_coord_new(const v8::Arguments &args);
void xnjsmb_coord_mkroot(v8::Handle<v8::Object> js_rt);

/* From shapes.cc */
void xnjsmb_shapes_init_mb_rt_temp(v8::Handle<v8::FunctionTemplate> rt_temp);


#endif /* __MBFLY_NJS_H_ */
