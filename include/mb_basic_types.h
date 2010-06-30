#ifndef __MB_BASIC_TYPES_H_
#define __MB_BASIC_TYPES_H_

typedef float co_aix;
typedef float co_comp_t;
typedef struct _grad_stop {
    co_aix offset;
    co_comp_t r, g, b, a;
} grad_stop_t;
typedef struct _area {
    co_aix x, y;
    co_aix w, h;
} area_t;

#endif /* __MB_BASIC_TYPES_H_ */
