// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <v8.h>
#include "mbfly_njs.h"

extern "C" {
#include <mb.h>
}

#ifndef ASSERT
#define ASSERT(x)
#endif

using namespace v8;

static Persistent<FunctionTemplate> xnjsmb_font_face_temp;

/*! \brief Query a font face.
 *
 * This function should be a method of rt object.
 *
 * \param family is family of font face.
 * \param slant is slant type of font face.
 * \param weight is weight of font face.
 */
static Handle<Value>
xnjsmb_font_face_query(const Arguments &args) {
    HandleScope scope;
    int argc = args.Length();
    Handle<Object> self = args.This();
    char *face;
    int slant, weight;
    redraw_man_t *rdman;
    mb_font_face_t *font_face;
    Handle<Function> func_obj;
    Handle<Object> face_obj;

    if(argc != 3)
	THROW("Invalid number of arguments (!= 3)");
    if(!args[0]->IsString() || !args[1]->IsInt32() || !args[2]->IsInt32())
	THROW("Invalid argument type");

    String::Utf8Value face_utf8(args[0]);
    face = *face_utf8;
    slant = args[1]->ToInt32()->Value();
    weight = args[2]->ToInt32()->Value();

    rdman = xnjsmb_rt_rdman(self);
    font_face = mb_font_face_query(rdman, face, slant, weight);
    if(font_face == NULL)
	return Null();

    func_obj = xnjsmb_font_face_temp->GetFunction();
    face_obj = func_obj->NewInstance();
    WRAP(face_obj, font_face);

    return face_obj;
}

static void
xnjsmb_font_face_init_temp(void) {
    Handle<FunctionTemplate> temp;
    Handle<ObjectTemplate> inst_temp;

    temp = FunctionTemplate::New();
    temp->SetClassName(String::New("font_face"));

    inst_temp = temp->InstanceTemplate();
    inst_temp->SetInternalFieldCount(1);

    xnjsmb_font_face_temp = Persistent<FunctionTemplate>::New(temp);
}

/*! \brief Add properties to the template of runtime objects.
 */
void
xnjsmb_font_init_mb_rt_temp(Handle<FunctionTemplate> mb_rt_temp) {
    HandleScope scope;
    static int init_flag = 0;
    Handle<ObjectTemplate> rt_proto_temp;
    Handle<FunctionTemplate> query_func_temp;

    if(!init_flag) {
	xnjsmb_font_face_init_temp();
	init_flag = 1;
    }

    rt_proto_temp = mb_rt_temp->PrototypeTemplate();
    query_func_temp = FunctionTemplate::New(xnjsmb_font_face_query);
    SET(rt_proto_temp, "font_face_query", query_func_temp);
}
