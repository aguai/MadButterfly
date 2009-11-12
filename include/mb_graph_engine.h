#ifndef __MBE_H_
#define __MBE_H_
#include <mb_config.h>

#ifdef CAIRO_GRAPH_ENGINE
#include <mb_graph_engine_cairo.h>
#endif

#ifdef SKAI_GRAPH_ENGINE
#include <mb_graph_engine_skia.h>
#endif

#endif /* __MBE_H_ */
