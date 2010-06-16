/*! \page create_graph_engine Create a Graphic Engine.
 *
 * To create a graphic engine, you need to declare and define types
 * and functions that had been declared in
 * include/mb_graph_engine_cairo.h in a separated header an c file.
 * Likes what mb_graph_engine_skia.h does.
 *
 * You should also add options in configure.ac to enable the graphic
 * engine.  You also need to add lines in include/mb_config.h.in and
 * include/mb_graph_engine.h to include correct header for the graphic
 * engine enabled by the user.
 */
#ifndef __MBE_H_
#define __MBE_H_
#include <mb_config.h>

#ifdef CAIRO_GRAPH_ENGINE
#include <mb_graph_engine_cairo.h>
#endif

#ifdef SKIA_GRAPH_ENGINE
#include <mb_graph_engine_skia.h>
#endif

#ifdef OPENVG_GRAPH_ENGINE
#include <mb_graph_engine_openvg.h>
#endif

#endif /* __MBE_H_ */
