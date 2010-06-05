#include <stdio.h>
#include <v8.h>

using namespace v8;

extern "C" void
init(Handle<Object> target) {
    HandleScope scope;

    target->Set(String::New("Hello"), String::New("World"));
}
