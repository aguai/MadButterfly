#ifndef __button_H_
#define __button_H_

typedef struct button button_t;

struct button {
    redraw_man_t *rdman;
    coord_t *root_coord;

    paint_t *linearGradient4066;

    paint_t *linearGradient4026;

    paint_t *linearGradient4018;

    coord_t *layer1;

    coord_t *normal;

    coord_t *rect6048_coord;

    shape_t *rect6048;

    paint_t *rect6048_fill;

    shape_t *rect6050;

    paint_t *rect6050_fill;

    shape_t *rect6052;

    paint_t *rect6052_fill;

    shape_t *tspan6056;

    paint_t *tspan6056_fill;

    coord_t *active;

    coord_t *rect4074_coord;

    shape_t *rect4074;

    paint_t *rect4074_fill;

    shape_t *rect3240;

    paint_t *rect3240_fill;

    shape_t *rect3242;

    paint_t *rect3242_fill;

    shape_t *tspan4387;

    paint_t *tspan4387_fill;

    coord_t *layer2;
};

extern button_t *button_new(redraw_man_t *rdman, coord_t *parent_coord);
extern void button_free(button_t *obj);

#endif /* __button_H_ */
