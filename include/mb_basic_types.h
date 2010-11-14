// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#ifndef __MB_BASIC_TYPES_H_
#define __MB_BASIC_TYPES_H_

typedef float co_aix;
typedef float co_comp_t;
typedef struct _grad_stop {
    co_aix offset;
    co_comp_t r, g, b, a;
} grad_stop_t;

#endif /* __MB_BASIC_TYPES_H_ */
