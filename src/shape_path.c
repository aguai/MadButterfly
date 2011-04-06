// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "mb_graph_engine.h"
#include "mb_types.h"
#include "mb_redraw_man.h"

/*! \brief Implement respective objects for SVG path tag.
 *
 * In user_data or dev_data, 0x00 bytes are padding after commands.
 * No commands other than 0x00 can resident after 0x00 itself.
 * It means command processing code can skip commands after a 0x00.
 *
 * Shapes should check if shape_t::geo is assigned.  Once transformation
 * matrics are changed, shape objects should update shape_t::geo if
 * it is assigned.
 */
typedef struct _sh_path {
    shape_t shape;
    int cmd_len;
    int pnt_len;
    int float_arg_len;
    char *user_data;
    char *dev_data;		/* device space data */
    
    redraw_man_t *rdman;	/*!< \brief This is used by sh_path_free() */
} sh_path_t;
#define RESERVED_AIXS sizeof(co_aix[2])

int _sh_path_size = sizeof(sh_path_t);

#define ASSERT(x)
#define SKIP_SPACE(x) while(*(x) && (isspace(*(x)) || *(x) == ',')) { (x)++; }
#define SKIP_NUM(x)					\
    while(*(x) &&					\
	  (isdigit(*(x)) ||				\
	   *(x) == 'e' ||				\
	   *(x) == 'E' ||				\
	   *(x) == '-' ||				\
	   *(x) == '+' ||				\
	   *(x) == '.'))  {				\
	(x)++;						\
    }
#define OK 0
#define ERR -1
#define PI 3.1415926535897931
#define FRAC_PI ((int)(PI * FRACTION_ONE))

#define SWAP(x, y) do { x ^= y; y ^= x; x ^= y; } while(0)
#define MAX(x, y) (((x) > (y))? (x): (y))
#define MIN(x, y) (((x) > (y))? (y): (x))
#define IS_NEGATIVE(x) ((x) < 0)

#ifdef UNITTEST
#undef rdman_man_shape
#define rdman_man_shape(x, y)

#undef elmpool_elm_alloc
#define elmpool_elm_alloc(pool) _elmpool_elm_alloc(pool)
static void *
_elmpool_elm_alloc(void *dummy) {
    return malloc(sizeof(sh_path_t));
}

#undef elmpool_elm_free
#define elmpool_elm_free(pool, elm) _elmpool_elm_free(pool, elm)
static void
_elmpool_elm_free(void *pool, void *elm) {
    free(elm);
}
#endif

/* ============================================================
 * Implement arc in path.
 */
#if 1

#include <stdint.h>
#include "precomputed.h"

#define ABS(x) (((x) > 0)? (x): -(x))
#define FRACTION_ONE (1 << FRACTION_SHIFT)

/*! \brief Compute the small slope of a vector.
 *
 * A small slope is based on absolute value of x-axis and y-axis.
 * And use smaller one of absolute values as divisor.
 */
static int
_small_slope(int x, int y) {
    int _x, _y;
    int r;

    _x = ABS(x);
    _y = ABS(y);
    if(_x > _y)
	r = (_y << FRACTION_SHIFT) / _x;
    else
	r = (_x << FRACTION_SHIFT) / _y;

    return r;
}

/*! \brief Index a given angle in slope table.
 *
 * Binary search.
 */
static int
_find_slope_index(int slope) {
    int left, right, v;

    left = 0;
    right = SLOPE_TAB_SZ - 1;
    while(left <= right) {
	v = (left + right) / 2;
	if(slope < slope_tab[v])
	    right = v - 1;
	else
	    left = v + 1;
    }

    return v;
}

static int
_vector_len(int x, int y) {
    int64_t _x, _y;
    int64_t slope;
    int64_t slope_index;
    int64_t radius;
    
    _x = ABS(x);
    _y = ABS(y);
    
    if(_x > _y) {
	slope = (_y << FRACTION_SHIFT) / _x;
	slope_index = _find_slope_index(slope);
	radius = _x * vector_len_factor_tab[slope_index];
    } else {
	slope = (_x << FRACTION_SHIFT) / _y;
	slope_index = _find_slope_index(slope);
	radius = _y * vector_len_factor_tab[slope_index];
    }
    radius = radius / FRACTION_ONE;
    
    return radius;
}

/*! \brief Find index of an arc-to-radius ratio in arc_radius_ratio_tab.
 *
 * Binary search.
 */
static int
_find_arc_radius(int arc_radius_ratio) {
    int left, right, v;

    left = 0;
    right = ARC_RADIUS_RATIO_TAB_SZ - 1;
    while(left <= right) {
	v = (left + right) / 2;
	if(arc_radius_ratio < arc_radius_ratio_tab[v])
	    right = v - 1;
	else
	    left = v + 1;
    }

    return v;
}

/* Compute shift factor for the ratio of arc to radius */
static int
_get_arc_radius_shift_factor(int arc_x, int arc_y, int radius) {
    int arc_len;
    int radius_len;
    int arc_radius_ratio;
    int arc_radius_index;
    int arc_radius_factor;
    
    arc_len = _vector_len(ABS(arc_x), ABS(arc_y));
    arc_radius_ratio = (arc_len << FRACTION_SHIFT) / radius;
    arc_radius_index = _find_arc_radius(arc_radius_ratio);
    
    arc_radius_factor = arc_radius_factor_tab[arc_radius_index];
    
    return arc_radius_factor;
}

/* Return a unit vector in the extend direction.
 *
 * This function make a decision on the direction of extend to make
 * radius of rx direction equivlant to ry direction.  It extends the
 * direction of short one.
 */
static void
_compute_extend_unit_vector(int rx, int ry, int x_rotate,
			    int64_t *ext_unit_x, int64_t *ext_unit_y) {
    int extend_dir;
    int extend_phase;
    int extend_index;
    int extend_sin, extend_cos;
    /* Change sign of x, y values accroding phase of the vector. */
    static int sin_cos_signs_tab[4][2] = {
	/* 0 for positive, 1 for negative */
	{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    int *signs;
    
    if(rx > ry)
	extend_dir = x_rotate + (FRAC_PI >> 1);
    else
	extend_dir = x_rotate;
    extend_dir %= FRAC_PI * 2;
    extend_phase = extend_dir / (FRAC_PI >> 1);
    
    extend_index = (extend_dir % (FRAC_PI >> 1)) * SIN_TAB_SZ /
	(FRAC_PI >> 1);
    if(extend_phase & 0x1)	/* half-phases 1,3 */
	extend_index = SIN_TAB_SZ - extend_index - 1;
    
    extend_sin = sin_tab[extend_index];
    extend_cos = sin_tab[SIN_TAB_SZ - extend_index - 1];
    
    signs = sin_cos_signs_tab[extend_phase];
    *ext_unit_x = signs[0]? -extend_cos: extend_cos;
    *ext_unit_y = signs[1]? -extend_sin: extend_sin;
}

static void
_get_center_ref_shift(int arc_x, int arc_y, int large, int sweep,
		      int slope_index,
		      int64_t *shift_cx, int64_t *shift_cy) {
    int _shift_cx, _shift_cy;
    int stat = 0;
    /* Change sign of shift-x/y accroding sign of arc_x, arc_y,
     * large and sweep.
     */
    static int shift_signs_tab[16][2] = {
	/* +x,+y   -x,+y   +x,-y   -x,-y */
	{1, 1}, {0, 1}, {1, 0}, {0, 0}, /* small, negative-angle */
	{0, 0}, {1, 0}, {0, 1}, {1, 1}, /* large, negative-angle */
	{0, 0}, {1, 0}, {0, 1}, {1, 1}, /* small, positive-angle */
	{1, 1}, {0, 1}, {1, 0}, {0, 0}  /* large, positive-angle */
    };
    
    _shift_cx = center_shift_tab[slope_index][0];
    _shift_cy = center_shift_tab[slope_index][1];
    if(ABS(arc_x) <= ABS(arc_y)) {
	SWAP(_shift_cx, _shift_cy);
	_shift_cx = -_shift_cx;
	_shift_cy = -_shift_cy;
    }
    
    if(IS_NEGATIVE(arc_x))
	stat |= 0x1;
    if(IS_NEGATIVE(arc_y))
	stat |= 0x2;
    if(large)
	stat |= 0x4;
    if(sweep)
	stat |= 0x8;
    if(shift_signs_tab[stat][0])
	_shift_cx = -_shift_cx;
    if(shift_signs_tab[stat][1])
	_shift_cy = -_shift_cy;

    *shift_cx = _shift_cx;
    *shift_cy = _shift_cy;
}

static int
_calc_center_i(int x0, int y0,
	       int x, int y,
	       int rx, int ry,
	       int x_rotate,
	       int large, int sweep,
	       int *cx, int *cy) {
    int64_t radius;
    int64_t ext_unit_y, ext_unit_x;	/* x and y value of unit vector on
				 * extend direction */
    int64_t arc_x, arc_y;
    int64_t radius_ref_ratio;
    int64_t arc_radius_factor;
    int64_t slope, slope_index;
    int64_t shift_cx, shift_cy;
    int64_t center_shift_factor;
    static int negatives[4] = {0, 1, 1, 0};
    int64_t extend_len;
    int64_t extend_x, extend_y;
    
    ASSERT(rx >= 0 && ry >= 0);
    
    arc_x = x - x0;
    arc_y = y - y0;
    
    if(arc_x == 0 && arc_y == 0) {
	*cx = x0;
	*cy = y0;
	return OK;
    }

    /* Translate arc to the coordinate that extend rx or ry to the
     * equivlant size as another.  It translate the ellipse to a
     * circle.
     */
    radius = MAX(rx, ry);
    _compute_extend_unit_vector(rx, ry, x_rotate, &ext_unit_x, &ext_unit_y);
    
    extend_len = (arc_x * ext_unit_x + arc_y * ext_unit_y) / FRACTION_ONE;
    extend_len = extend_len * (MAX(rx, ry) - MIN(rx, ry)) / MIN(rx, ry);
    extend_x = ext_unit_x * extend_len / FRACTION_ONE;
    extend_y = ext_unit_y * extend_len / FRACTION_ONE;
    
    arc_x += extend_x;
    arc_y += extend_y;
    
    /* Find range index of slope. */
    slope = _small_slope(arc_x, arc_y);
    slope_index = _find_slope_index(slope);

    /* Compute shift factor for the ratio of arc to radius */
    arc_radius_factor = _get_arc_radius_shift_factor(arc_x, arc_y, radius);

    /* Compute ratio of radius to reference radius */
    radius_ref_ratio = radius >> REF_RADIUS_SHIFT;
    
    /* Compute x/y-shift of center range index according
     * slope_index, radius_ref_ratio and arc_radius_factor.
     */
    _get_center_ref_shift(arc_x, arc_y, large, sweep, slope_index,
			  &shift_cx, &shift_cy);
    center_shift_factor = radius_ref_ratio * arc_radius_factor;
    center_shift_factor = center_shift_factor / FRACTION_ONE;
    shift_cx = shift_cx * center_shift_factor / FRACTION_ONE;
    shift_cy = shift_cy * center_shift_factor / FRACTION_ONE;
    
    shift_cx += arc_x / 2;
    shift_cy += arc_y / 2;

    /* translate shift_cx/cy back to original coordinate */
    extend_len = (shift_cx * ext_unit_x + shift_cy * ext_unit_y) /
	FRACTION_ONE;
    extend_len = extend_len * (MAX(rx, ry) - MIN(rx, ry)) / MAX(rx, ry);
    extend_x = ext_unit_x * extend_len / FRACTION_ONE;
    extend_y = ext_unit_y * extend_len / FRACTION_ONE;
    shift_cx = shift_cx - extend_x;
    shift_cy = shift_cy - extend_y;
    
    /* get center */
    *cx = x0 + shift_cx;
    *cy = y0 + shift_cy;

    return OK;
}

static int _calc_center(co_aix x0, co_aix y0,
			co_aix x, co_aix y,
			co_aix rx, co_aix ry,
			co_aix x_rotate,
			int large, int sweep,
			co_aix *cx, co_aix *cy) {
    int cx_i, cy_i;
    int r;
    
    r = _calc_center_i(x0 * FRACTION_ONE, y0 * FRACTION_ONE,
		       x * FRACTION_ONE, y * FRACTION_ONE,
		       rx * FRACTION_ONE, ry * FRACTION_ONE,
		       x_rotate * FRACTION_ONE,
		       large, sweep, &cx_i, &cy_i);
    *cx = (co_aix)cx_i / FRACTION_ONE;
    *cy = (co_aix)cy_i / FRACTION_ONE;
    return r;
}

#else
/*! \brief Calculate center of the ellipse of an arc.
 *
 * Origin of our coordination is left-top corner, and y-axis are grown
 * to down-side.
 *
 * Space of the arc is transformed to space that correspondent
 * ellipse containing the arc is mapped into an unit circle.
 * - ux0 = x0 / rx
 * - uy0 = y0 / ry
 * - ux = x / rx
 * - uy = y / ry
 * ux0, uy0, ux, uy are got by transforming (x0, y0) and (x, y) into points
 * on the unit circle. The center of unit circle are (ucx, ucy):
 * - umx = (ux0 + ux) / 2
 * - umy = (uy0 + uy) / 2
 * - udcx = ucx - umx
 * - udcy = ucy - umy
 * - udx = ux - umx
 * - udy = uy - umy
 *
 * - udx * udcx + udy * udcy = 0
 *
 * - udl2 = udx ** 2 + udy ** 2
 *
 * For drawing small arc in clockwise
 * - udx * udcy - udy * udcx = sqrt((1 - udl2) * udl2)
 *
 * - udcy = -udcx * udx / udy
 * - -udcx * (udx ** 2) / udy - udy * udcx = sqrt((1 - udl2) * udl2)
 * - -udcx * ((udx ** 2) / udy + udy) = sqrt((1 - udl2) * udl2)
 * - udcx = -sqrt((1 - udl2) * udl2) / ((udx ** 2) / udy + udy)
 * or
 * - udcx = -udcy * udy / udx
 * - udx * udcy + udcy * (udy ** 2) / udx = sqrt((1 - udl2) * udl2)
 * - udcy * (udx + (udy ** 2) / udx) = sqrt((1 - udl2) * udl2)
 * - udcy = sqrt((1 - udl2) * udl2) / (udx + (udy ** 2) / udx)
 *
 * - cx = rx * ucx
 * - cx = rx * (udcx + umx)
 * - cy = ry * ucy
 * - cy = ry * (udcy + umy)
 */
static int _calc_center(co_aix x0, co_aix y0,
		       co_aix x, co_aix y,
		       co_aix rx, co_aix ry,
		       co_aix x_rotate,
		       int large, int sweep,
		       co_aix *cx, co_aix *cy) {
    co_aix br_x, br_y, br_x0, br_y0; /* before-rotated x, y, x0, y0 */
    co_aix udx, udy, udx2, udy2;
    co_aix umx, umy;
    co_aix udcx, udcy;
    co_aix br_cx, br_cy;
    co_aix udl2;
    co_aix rev_rx2, rev_ry2;
    float _sin = -sinf(x_rotate); /* rotate to oposite direction */
    float _cos = cosf(x_rotate);
    int reflect;

#define X_AFTER_ROTATE(x, y, sin, cos) (x * cos - y * sin)
#define Y_AFTER_ROTATE(x, y, sin, cos) (x * sin + y * cos)

    /* Restore positions to the value before rotation */
    br_x = X_AFTER_ROTATE(x, y, _sin, _cos);
    br_y = Y_AFTER_ROTATE(x, y, _sin, _cos);
    br_x0 = X_AFTER_ROTATE(x0, y0, _sin, _cos);
    br_y0 = Y_AFTER_ROTATE(x0, y0, _sin, _cos);

    /* Resize to be an unit circle */
    rev_rx2 = 1.0 / (2 * rx);
    rev_ry2 = 1.0 / (2 * ry);
    udx = (br_x - br_x0) * rev_rx2; /* ux - umx */
    udy = (br_y - br_y0) * rev_ry2; /* uy - umy */
    umx = (br_x + br_x0) * rev_rx2;
    umy = (br_y + br_y0) * rev_ry2;

    udx2 = udx * udx;
    udy2 = udy * udy;
    udl2 = udx2 + udy2;

    if(udy != 0) {
	/* center is at left-side of arc */
	udcx = -sqrtf((1 - udl2) * udl2) / (udy + udx2 / udy);
	udcy = -udcx * udx / udy;
    } else {
	/* center is at down-side of arc */
	udcx = 0;
	udcy = sqrtf((1 - udl2) * udl2) / udx;
    }

    reflect = 0;
    if(large)
	reflect ^= 1;
    if(sweep != 1)
	reflect ^= 1;
    if(reflect) {
	udcx = -udcx;
	udcy = -udcy;
    }

    br_cx = rx * (udcx + umx);
    br_cy = ry * (udcy + umy);

    *cx = X_AFTER_ROTATE(br_cx, br_cy, -_sin, _cos);
    *cy = Y_AFTER_ROTATE(br_cx, br_cy, -_sin, _cos);

    return OK;
}
#endif

static co_aix _angle_rotated_ellipse(co_aix x, co_aix y,
				     co_aix rx, co_aix ry,
				     co_aix x_rotate) {
    co_aix nrx, nry;
    co_aix _sin, _cos;
    co_aix xy_tan;
    co_aix angle;

    _sin = sinf(x_rotate);
    _cos = cosf(x_rotate);

    nrx = (x * _cos + y * _sin) / rx;
    nry = (-x * _sin + y * _cos) / ry;
    xy_tan = nry / nrx;

    angle = atan(xy_tan);

    if(nrx < 0)
	angle = PI + angle;

    return angle;
}

static void _rotate(co_aix *x, co_aix *y, co_aix _sin, co_aix _cos) {
    co_aix nx, ny;

    nx = *x * _cos - *y * _sin;
    ny = *x * _sin + *y * _cos;

    *x = nx;
    *y = ny;
}

#define TAKE_NUM(r) do {			\
	SKIP_SPACE(p);				\
	old = p;				\
	SKIP_NUM(p);				\
	if(p == old)				\
	    return ERR;				\
	r = atof(old);				\
    } while(0);

static int _sh_path_arc_cmd_arg_fill(char cmd, char **cmds_p,
				     const char **data_p,
				     co_aix **pnts_p,
				     co_aix **float_args_p) {
    co_aix rx, ry;
    co_aix x_rotate;
    int large, sweep;
    co_aix x, y, x0, y0, cx, cy;
    co_aix corners[4][2];
    co_aix angle_start, angle_stop;
    co_aix *pnts = *pnts_p;
    const char *old;
    const char *p;
    char *cmds;
    co_aix *float_args;
    co_aix _sin, _cos;
    int i;

    p = *data_p;
    cmds = *cmds_p;
    float_args = *float_args_p;
    while(*p) {
	SKIP_SPACE(p);
	old = p;
	SKIP_NUM(p);
	if(p == old)
	    break;
	rx = atof(old);

	TAKE_NUM(ry);
	TAKE_NUM(x_rotate);
	TAKE_NUM(large);
	TAKE_NUM(sweep);
	TAKE_NUM(x);
	TAKE_NUM(y)

	x0 = *(pnts - 2);
	y0 = *(pnts - 1);

	if(islower(cmd)) {
	    x += x0;
	    y += y0;
	}

	_calc_center(x0, y0, x, y, rx, ry, x_rotate, large,
		     sweep, &cx, &cy);

	/* Compute positions for four corners.
	 * These four corners form a bounding box for the arc.
	 */
	_sin = sinf(x_rotate);
	_cos = cosf(x_rotate);
	corners[0][0] = -rx;
	corners[0][1] = -ry;
	corners[1][0] = rx;
	corners[1][1] = -ry;
	corners[2][0] = rx;
	corners[2][1] = ry;
	corners[3][0] = -rx;
	corners[3][1] = ry;
	for(i = 0; i < 4; i++) {
	    _rotate(&corners[i][0], &corners[i][1], _sin, _cos);
	    *pnts++ = corners[i][0] + cx;
	    *pnts++ = corners[i][1] + cy;
	}

	*(pnts++) = x;
	*(pnts++) = y;

	angle_start = _angle_rotated_ellipse(x0 - cx, y0 - cy,
					     rx, ry, x_rotate);
	angle_stop = _angle_rotated_ellipse(x - cx, y - cy,
					    rx, ry, x_rotate);

	if(sweep && angle_start > angle_stop)
	    angle_stop += 2 * PI;
	else if((!sweep) && angle_start < angle_stop)
	    angle_start += 2 * PI;

	*float_args++ = cx;
	*float_args++ = cy;
	*float_args++ = rx;
	*float_args++ = ry;
	*float_args++ = angle_start;
	*float_args++ = angle_stop;
	*float_args++ = x_rotate;

	*cmds++ = toupper(cmd);
    }

    *data_p = p;
    *pnts_p = pnts;
    *cmds_p = cmds;
    *float_args_p = float_args;

    return OK;
}

#define INNER(x1, y1, x2, y2) ((x1) * (x2) + (y1) * (y2))
#define CROSS(x1, y1, x2, y2) ((x1) * (y2) - (y1) * (x2))

static co_aix distance_pow2(co_aix x, co_aix y) {
    return x * x + y * y;
}

static co_aix angle_diff(co_aix sx, co_aix sy, co_aix dx, co_aix dy) {
    co_aix inner, cross;
    co_aix angle;
    co_aix rd2, rd;

    rd2 = distance_pow2(dx, dy);
    rd = sqrtf(rd2);

    inner = INNER(sx, sy, dx, dy);
    cross = CROSS(sx, sy, dx, dy);
    angle = acos(inner / rd);
    if(cross < 0)
	angle = 2 * PI - angle;

    return angle;
}

/*! \brief Make path for arcs in a path.
 */
void _sh_path_arc_path(mbe_t *cr, sh_path_t *path, const co_aix **pnts_p,
		       const co_aix **float_args_p) {
    co_aix cx, cy, x0, y0, x, y;
    co_aix rx, ry;
    co_aix xyratio;
    co_aix angle_start, angle_stop;
    co_aix x_rotate;
    const co_aix *pnts;
    const co_aix *float_args;
    co_aix matrix[6];
    co_aix dev_matrix[6];
    co_aix *aggr;
    co_aix _sin, _cos;

    pnts = *pnts_p;
    float_args = *float_args_p;
    x0 = *(pnts - 2);
    y0 = *(pnts - 1);
    pnts += 8;
    x = *pnts++;
    y = *pnts++;

    cx = *float_args++;
    cy = *float_args++;
    rx = *float_args++;
    ry = *float_args++;
    angle_start = *float_args++;
    angle_stop = *float_args++;
    x_rotate = *float_args++;

    _sin = sinf(x_rotate);
    _cos = cosf(x_rotate);

    xyratio = ry / rx;
    aggr = sh_get_aggr_matrix((shape_t *)path);
    matrix[0] = _cos;
    matrix[1] = -_sin * xyratio;
    matrix[2] = cx;
    matrix[3] = _sin;
    matrix[4] = _cos * xyratio;
    matrix[5] = cy;

    matrix_mul(aggr, matrix, dev_matrix);
    mbe_save(cr);
    mbe_transform(cr, dev_matrix);
    mbe_arc(cr, 0, 0, rx, angle_start, angle_stop);
    mbe_restore(cr);

    *pnts_p = pnts;
    *float_args_p = float_args;
}

/* ============================================================ */

static void sh_path_free(shape_t *shape) {
    sh_path_t *path = (sh_path_t *)shape;

    mb_obj_destroy(path);
    if(path->user_data)
	free(path->user_data);
    elmpool_elm_free(path->rdman->sh_path_pool, path);
}

/*! \brief Count number of arguments.
 *
 * \todo Notify programmers that syntax or value error of path data.
 */
static int sh_path_cmd_arg_cnt(const char *data, int *cmd_cntp, int *pnt_cntp,
			       int *float_arg_cntp) {
    const char *p, *old;
    int cmd_cnt, pnt_cnt, float_arg_cnt;
    int i;

    cmd_cnt = pnt_cnt = float_arg_cnt = 0;
    p = data;
    SKIP_SPACE(p);
    while(*p) {
	switch(*p++) {
	case 'c':
	case 'C':
	    while(*p) {
		old = p;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    break;
		pnt_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		pnt_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		pnt_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		pnt_cnt++;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		pnt_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		pnt_cnt++;

		cmd_cnt++;
	    }
	    break;
	case 's':
	case 'S':
	case 'q':
	case 'Q':
	    while(*p) {
		old = p;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    break;
		pnt_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		pnt_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		pnt_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		pnt_cnt++;

		cmd_cnt++;
	    }
	    break;
	case 'm':
	case 'M':
	case 'l':
	case 'L':
	case 't':
	case 'T':
	    while(*p) {
		old = p;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    break;
		pnt_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		pnt_cnt++;

		cmd_cnt++;
	    }
	    break;
	case 'h':
	case 'H':
	case 'v':
	case 'V':
	    while(*p) {
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    break;
		pnt_cnt += 2;

		cmd_cnt++;
	    }
	    break;
	case 'A':
	case 'a':
	    while(*p) {
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    break;

		for(i = 0; i < 6; i++) {
		    SKIP_SPACE(p);
		    old = p;
		    SKIP_NUM(p);
		    if(p == old)
			return ERR;
		}

		pnt_cnt += 10;
		float_arg_cnt += 7;

		cmd_cnt++;
	    }
	    break;
	case 'z':
	case 'Z':
	    cmd_cnt++;
	    break;
	default:
	    return ERR;
	}
	/*! \todo cmd_cnt should be increased for each implicit repeating. */
	SKIP_SPACE(p);
    }

    *cmd_cntp = cmd_cnt;
    *pnt_cntp = pnt_cnt;
    *float_arg_cntp = float_arg_cnt;
    return OK;
}

#define TO_ABSX islower(cmd)? x + atof(old): atof(old)
#define TO_ABSY islower(cmd)? y + atof(old): atof(old)

static int sh_path_cmd_arg_fill(const char *data, sh_path_t *path) {
    const char *p, *old;
    char *cmds;
    char cmd;
    co_aix *pnts;
    co_aix *float_args;
    co_aix sx = 0, sy = 0;
    co_aix x, y;
    int r;

    cmds = path->user_data;
    pnts = (co_aix *)(cmds + path->cmd_len);
    float_args = (co_aix *)(cmds + path->cmd_len +
			    path->pnt_len * sizeof(co_aix));
    p = data;
    SKIP_SPACE(p);
    while(*p) {
	/* Transform all relative to absolute, */
	x = *(pnts - 2);
	y = *(pnts - 1);

	switch((cmd = *p++)) {
	case 'c':
	case 'C':
	    while(*p) {
		old = p;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    break;
		*pnts = TO_ABSX;
		pnts++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSY;
		pnts++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSX;
		pnts++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSY;
		pnts++;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSX;
		x = *pnts;
		pnts++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSY;
		y = *pnts;
		pnts++;

		*cmds++ = toupper(cmd);
	    }
	    break;
	case 's':
	case 'S':
	case 'q':
	case 'Q':
	    while(*p) {
		old = p;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    break;
		*pnts = TO_ABSX;
		pnts++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSY;
		pnts++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSX;
		x = *pnts;
		pnts++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSY;
		y = *pnts;
		pnts++;

		*cmds++ = toupper(cmd);
	    }
	    break;

	case 'm':
	case 'M':
	    while(*p) {
		old = p;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    break;
		*pnts = TO_ABSX;
		x = *pnts;
		pnts++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSY;
		y = *pnts;
		pnts++;

		*cmds++ = toupper(cmd);

		/* save initial point of a subpath */
		sx = x;
		sy = y;
	    }
	    break;

	case 'l':
	case 'L':
	case 't':
	case 'T':
	    while(*p) {
		old = p;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    break;
		*pnts = TO_ABSX;
		x = *pnts;
		pnts++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*pnts = TO_ABSY;
		y = *pnts;
		pnts++;

		*cmds++ = toupper(cmd);
	    }
	    break;
	case 'h':
	case 'H':
	case 'v':
	case 'V':
	    /*! \todo implement h, H, v, V comamnds for path. */
	    return ERR;

	case 'A':
	case 'a':
	    r = _sh_path_arc_cmd_arg_fill(cmd, &cmds,
					  (const char **)&p, &pnts,
					  &float_args);
	    if(r != OK)
		return ERR;
	    break;

	case 'z':
	case 'Z':
	    *cmds++ = toupper(cmd);
	    /* Go back to initial point of a subpath */
	    x = sx;
	    y = sy;
	    break;
	default:
	    return ERR;
	}
	SKIP_SPACE(p);
    }

    return OK;
}

/*! \brief Create a path from value of 'data' of SVG path.
 */
shape_t *rdman_shape_path_new(redraw_man_t *rdman, const char *data) {
    sh_path_t *path;
    int cmd_cnt, pnt_cnt, float_arg_cnt;
    int msz;
    int r;

    r = sh_path_cmd_arg_cnt(data, &cmd_cnt, &pnt_cnt,
			    &float_arg_cnt);
    if(r == ERR)
	return NULL;

    /* Align at 4's boundary and keep 2 unused co_aix space
     * to make logic of transformation from relative to absolute
     * simple.
     */
    cmd_cnt += RESERVED_AIXS;
    cmd_cnt = (cmd_cnt + 3) & ~0x3;

    /*! \todo Use elmpool to manage sh_path_t objects. */
    path = (sh_path_t *)elmpool_elm_alloc(rdman->sh_path_pool);
    /*! \todo Remove this memset()? */
    memset(&path->shape, 0, sizeof(shape_t));
    mb_obj_init(path, MBO_PATH);
    path->cmd_len = cmd_cnt;
    path->pnt_len = pnt_cnt;
    path->float_arg_len = float_arg_cnt;

    msz = cmd_cnt + sizeof(co_aix) * pnt_cnt +
	sizeof(co_aix) * float_arg_cnt;
    path->user_data = (char *)malloc(msz * 2);
    if(path->user_data == NULL) {
	elmpool_elm_free(rdman->sh_path_pool, path);
	return NULL;
    }

    path->dev_data = path->user_data + msz;

    r = sh_path_cmd_arg_fill(data, path);
    if(r == ERR) {
	free(path->user_data);
	elmpool_elm_free(rdman->sh_path_pool, path);
	return NULL;
    }
    memcpy(path->dev_data, path->user_data, msz);

    path->shape.free = sh_path_free;
    path->rdman = rdman;

    rdman_man_shape(rdman, (shape_t *)path);

    return (shape_t *)path;
}

shape_t *rdman_shape_path_new_from_binary(redraw_man_t *rdman,
					  char *commands,
					  co_aix *pnts, int pnt_cnt,
					  co_aix *float_args,
					  int float_arg_cnt) {
    sh_path_t *path;
    int msz;
    int cmd_cnt = strlen(commands);

    /*! \todo Use elmpool to manage sh_path_t objects. */
    path = (sh_path_t *)elmpool_elm_alloc(rdman->sh_path_pool);
    /*! \todo Remove this memset()? */
    memset(&path->shape, 0, sizeof(shape_t));
    mb_obj_init(path, MBO_PATH);
    cmd_cnt = (cmd_cnt + 3) & ~0x3;
    path->cmd_len = cmd_cnt;
    path->pnt_len = pnt_cnt;
    path->float_arg_len = float_arg_cnt;
    msz = cmd_cnt + sizeof(co_aix) * pnt_cnt +
	sizeof(co_aix) * float_arg_cnt;
    path->user_data = (char *)malloc(msz * 2);
    if(path->user_data == NULL) {
	elmpool_elm_free(rdman->sh_path_pool, path);
	return NULL;
    }

    path->dev_data = path->user_data + msz;
    memcpy(path->user_data, commands, strlen(commands) + 1);
    memcpy(path->user_data + cmd_cnt, pnts, sizeof(co_aix) * pnt_cnt);
    memcpy(path->user_data + cmd_cnt + pnt_cnt * sizeof(co_aix),
	   float_args, sizeof(co_aix) * float_arg_cnt);
    memcpy(path->dev_data, path->user_data, msz);

    path->shape.free = sh_path_free;
    path->rdman = rdman;

    rdman_man_shape(rdman, (shape_t *)path);

    return (shape_t *)path;
}

shape_t *
rdman_shape_path_clone(redraw_man_t *rdman, const shape_t *_src_path) {
    const sh_path_t *src_path = (const sh_path_t *)_src_path;
    sh_path_t *new_path;
    char *udata;
    char *cmds;
    co_aix *pnts, *float_args;

    udata = src_path->user_data;
    cmds = udata;
    pnts = (co_aix *)(udata + src_path->cmd_len);
    float_args = pnts + src_path->pnt_len;
    new_path =
	(sh_path_t *)rdman_shape_path_new_from_binary(rdman,
						      cmds,
						      pnts, src_path->pnt_len,
						      float_args,
						      src_path->float_arg_len);
    if(new_path == NULL)
	return NULL;

    sh_copy_style(rdman, (shape_t *)src_path, (shape_t *)new_path);

    return (shape_t *)new_path;
}


/*! \brief Transform a path from user space to device space.
 *
 */
void sh_path_transform(shape_t *shape) {
    sh_path_t *path;
    co_aix *pnts, *dev_pnts;
    co_aix (*poses)[2];
    area_t *area;
    int pnt_len;
    int i;

    ASSERT(shape->type == SHT_PATH);
    ASSERT((shape->pnt_len & 0x1) == 0);

    path = (sh_path_t *)shape;
    pnts = (co_aix *)(path->user_data + path->cmd_len);
    dev_pnts = (co_aix *)(path->dev_data + path->cmd_len);
    pnt_len = path->pnt_len;
    for(i = 0; i < pnt_len; i += 2) {
	dev_pnts[0] = *pnts++;
	dev_pnts[1] = *pnts++;
	coord_trans_pos(shape->coord, dev_pnts, dev_pnts + 1);
	dev_pnts += 2;
    }

    if(path->shape.geo) {
	poses = (co_aix (*)[2])(path->dev_data + path->cmd_len);
	geo_from_positions(path->shape.geo, pnt_len / 2, poses);
	area = shape->geo->cur_area;
	area->x -= shape->stroke_width / 2 + 0.5;
	area->y -= shape->stroke_width / 2 + 0.5;
	area->w += shape->stroke_width + 1;
	area->h += shape->stroke_width + 1;
    }
}

static void sh_path_path(shape_t *shape, mbe_t *cr) {
    sh_path_t *path;
    int cmd_len;
    char *cmds, cmd;
    const co_aix *pnts;
    const co_aix *float_args;
    co_aix x, y, x1, y1, x2, y2;
    int i;

    ASSERT(shape->type == SHT_PATH);

    path = (sh_path_t *)shape;
    cmd_len = path->cmd_len;
    cmds = path->dev_data;
    pnts = (co_aix *)(cmds + cmd_len);
    float_args = (co_aix *)(cmds + cmd_len + path->pnt_len * sizeof(co_aix));
    x = y = x1 = y1 = x2 = y2 = 0;
    for(i = 0; i < cmd_len; i++) {
	/* All path commands and arguments are transformed
	 * to absoluted form.
	 */
	cmd = *cmds++;
	switch(cmd) {
	case 'M':
	    x = *pnts++;
	    y = *pnts++;
	    mbe_move_to(cr, x, y);
	    break;
	case 'L':
	    x = *pnts++;
	    y = *pnts++;
	    mbe_line_to(cr, x, y);
	    break;
	case 'C':
	    x1 = *pnts++;
	    y1 = *pnts++;
	    x2 = *pnts++;
	    y2 = *pnts++;
	    x = *pnts++;
	    y = *pnts++;
	    mbe_curve_to(cr, x1, y1, x2, y2, x, y);
	    break;
	case 'S':
	    x1 = x + x - x2;
	    y1 = y + y - y2;
	    x2 = *pnts++;
	    y2 = *pnts++;
	    x = *pnts++;
	    y = *pnts++;
	    mbe_curve_to(cr, x1, y1, x2, y2, x, y);
	    break;
	case 'Q':
	    x1 = *pnts++;
	    y1 = *pnts++;
	    x2 = x1;
	    y2 = y1;
	    x = *pnts++;
	    y = *pnts++;
	    mbe_curve_to(cr, x1, y1, x2, y2, x, y);
	    break;
	case 'T':
	    x1 = x + x - x2;
	    y1 = y + y - y2;
	    x2 = x1;
	    y2 = y1;
	    x = *pnts++;
	    y = *pnts++;
	    mbe_curve_to(cr, x1, y1, x2, y2, x, y);
	    break;
	case 'A':
	    _sh_path_arc_path(cr, path, &pnts, &float_args);
	    break;
	case 'Z':
	    mbe_close_path(cr);
	    break;
	case '\x0':
	    i = cmd_len;	/* padding! Skip remain ones. */
	    break;
	}
    }
}

void sh_path_draw(shape_t *shape, mbe_t *cr) {
    sh_path_path(shape, cr);
}

#ifdef UNITTEST

#include <CUnit/Basic.h>

void test_rdman_shape_path_new(void) {
    sh_path_t *path;
    co_aix *pnts;
    redraw_man_t rdman;

    path = (sh_path_t *)rdman_shape_path_new(&rdman, "M 33 25l33 55c 33 87 44 22 55 99L33 77z");
    CU_ASSERT(path != NULL);
    CU_ASSERT(path->cmd_len == ((5 + RESERVED_AIXS + 3) & ~0x3));
    CU_ASSERT(path->pnt_len == 12);
    CU_ASSERT(strncmp(path->user_data, "MLCLZ", 5) == 0);
    CU_ASSERT(strncmp(path->dev_data, "MLCLZ", 5) == 0);

    pnts = (co_aix *)(path->user_data + path->cmd_len);
    CU_ASSERT(pnts[0] == 33);
    CU_ASSERT(pnts[1] == 25);
    CU_ASSERT(pnts[2] == 66);
    CU_ASSERT(pnts[3] == 80);
    CU_ASSERT(pnts[4] == 99);
    CU_ASSERT(pnts[5] == 167);
    CU_ASSERT(pnts[6] == 110);
    CU_ASSERT(pnts[7] == 102);
    CU_ASSERT(pnts[8] == 121);
    CU_ASSERT(pnts[9] == 179);
    CU_ASSERT(pnts[10] == 33);
    CU_ASSERT(pnts[11] == 77);
    sh_path_free((shape_t *)path);
}

void test_path_transform(void) {
    sh_path_t *path;
    co_aix *pnts;
    coord_t coord;
    geo_t geo;
    redraw_man_t rdman;

    path = (sh_path_t *)rdman_shape_path_new(&rdman, "M 33 25l33 55C 33 87 44 22 55 99L33 77z");
    CU_ASSERT(path != NULL);
    CU_ASSERT(path->cmd_len == ((5 + RESERVED_AIXS + 3) & ~0x3));
    CU_ASSERT(path->pnt_len == 12);
    CU_ASSERT(strncmp(path->user_data, "MLCLZ", 5) == 0);
    CU_ASSERT(strncmp(path->dev_data, "MLCLZ", 5) == 0);

    geo_init(&geo);
    path->shape.geo = &geo;
    geo.shape = (shape_t *)path;

    coord.aggr_matrix[0] = 1;
    coord.aggr_matrix[1] = 0;
    coord.aggr_matrix[2] = 1;
    coord.aggr_matrix[3] = 0;
    coord.aggr_matrix[4] = 2;
    coord.aggr_matrix[5] = 0;
    path->shape.coord = &coord;
    sh_path_transform((shape_t *)path);

    pnts = (co_aix *)(path->dev_data + path->cmd_len);
    CU_ASSERT(pnts[0] == 34);
    CU_ASSERT(pnts[1] == 50);
    CU_ASSERT(pnts[2] == 67);
    CU_ASSERT(pnts[3] == 160);
    CU_ASSERT(pnts[4] == 34);
    CU_ASSERT(pnts[5] == 174);
    CU_ASSERT(pnts[6] == 45);
    CU_ASSERT(pnts[7] == 44);
    CU_ASSERT(pnts[8] == 56);
    CU_ASSERT(pnts[9] == 198);
    CU_ASSERT(pnts[10] == 34);
    CU_ASSERT(pnts[11] == 154);

    sh_path_free((shape_t *)path);
}

void test_small_slope(void) {
    co_aix x, y;
    co_aix slope;
    co_aix r;

    x = 135.3;
    y = 149.6;
    r = (co_aix)_small_slope(x * FRACTION_ONE,
			     y * FRACTION_ONE) /
	FRACTION_ONE;

    slope = MIN(x, y) / MAX(x, y);
    CU_ASSERT(((r - slope) / slope) < 0.01 &&
	      ((r - slope) / slope) > -0.01);
}

void test_find_slope_index(void) {
    co_aix slope;
    int idx;
    co_aix r;

    slope = 0.754;
    idx = _find_slope_index(slope * FRACTION_ONE);
    r = (co_aix)slope_tab[idx] / FRACTION_ONE;
    CU_ASSERT((r / slope) < 1.01 &&
	      (r / slope) > 0.99);
}

void test_vector_len(void) {
    co_aix x, y;
    co_aix len;
    co_aix r;
    int rlen;

    x = 397;
    y = 449;
    len = sqrt(x * x + y * y);
    rlen = _vector_len(x * FRACTION_ONE,
		       y * FRACTION_ONE);
    r = (co_aix)rlen / (1 <<FRACTION_SHIFT);
    CU_ASSERT((r / len) < 1.01 &&
	      (r / len) > 0.99);

    x = 357;
    y = 224;
    len = sqrt(x * x + y * y);
    rlen = _vector_len(x * FRACTION_ONE,
		       y * FRACTION_ONE);
    r = (co_aix)rlen / FRACTION_ONE;
    CU_ASSERT((r / len) < 1.01 &&
	      (r / len) > 0.99);
}

void test_find_arc_radius(void) {
    co_aix ratio;
    int idx;
    co_aix r;

    ratio = 0.732;
    idx = _find_arc_radius(ratio * FRACTION_ONE);
    r = (co_aix)arc_radius_ratio_tab[idx] / FRACTION_ONE;
    CU_ASSERT((r / ratio) < 1.01 &&
	      (r / ratio) > 0.99);
}

void test_get_arc_radius_shift_factor(void) {
    co_aix arc_x, arc_y, radius;
    co_aix factor;
    int rfactor;
    co_aix r;

    arc_x = 30.5;
    arc_y = 10.3;
    radius = 90.3;
    factor = sqrt(radius * radius - (arc_x * arc_x + arc_y * arc_y) / 4) /
	radius;
    rfactor = _get_arc_radius_shift_factor(arc_x * FRACTION_ONE,
					   arc_y * FRACTION_ONE,
					   radius * FRACTION_ONE);
    r = (co_aix)rfactor / FRACTION_ONE;
    CU_ASSERT((r / factor) < 1.01 &&
	      (r / factor) > 0.99);

    arc_x = 30.5;
    arc_y = 70.3;
    radius = 190.3;
    factor = sqrt(radius * radius - (arc_x * arc_x + arc_y * arc_y) / 4) /
	radius;
    rfactor = _get_arc_radius_shift_factor(arc_x * FRACTION_ONE,
					   arc_y * FRACTION_ONE,
					   radius * FRACTION_ONE);
    r = (co_aix)rfactor / FRACTION_ONE;
    CU_ASSERT((r / factor) < 1.01 &&
	      (r / factor) > 0.99);
}

void test_compute_extend_unit_vector(void) {
    co_aix rx, ry;
    co_aix x_rotate;
    co_aix unit_x, unit_y;
    co_aix runit_x, runit_y;
    int64_t ext_unit_x, ext_unit_y;

    rx = 200;
    ry = 153;
    x_rotate = PI * 30 / 180;
    unit_x = cos(PI * 90 / 180 + x_rotate);
    unit_y = sin(PI * 90 / 180 + x_rotate);
    _compute_extend_unit_vector(rx * FRACTION_ONE, ry * FRACTION_ONE,
				x_rotate * FRACTION_ONE,
				&ext_unit_x, &ext_unit_y);
    runit_x = (co_aix)ext_unit_x / FRACTION_ONE;
    runit_y = (co_aix)ext_unit_y / FRACTION_ONE;
    CU_ASSERT((runit_x / unit_x) < 1.01 &&
	      (runit_x / unit_x) > 0.99);
    CU_ASSERT((runit_y / unit_y) < 1.01 &&
	      (runit_y / unit_y) > 0.99);

    rx = 200;
    ry = 153;
    x_rotate = PI * 158 / 180;
    unit_x = cos(PI * 90 / 180 + x_rotate);
    unit_y = sin(PI * 90 / 180 + x_rotate);
    _compute_extend_unit_vector(rx * FRACTION_ONE, ry * FRACTION_ONE,
				x_rotate * FRACTION_ONE,
				&ext_unit_x, &ext_unit_y);
    runit_x = (co_aix)ext_unit_x / FRACTION_ONE;
    runit_y = (co_aix)ext_unit_y / FRACTION_ONE;
    CU_ASSERT((runit_x / unit_x) < 1.01 &&
	      (runit_x / unit_x) > 0.99);
    CU_ASSERT((runit_y / unit_y) < 1.01 &&
	      (runit_y / unit_y) > 0.99);

    rx = 100;
    ry = 153;
    x_rotate = PI * 158 / 180;
    unit_x = cos(x_rotate);
    unit_y = sin(x_rotate);
    _compute_extend_unit_vector(rx * FRACTION_ONE, ry * FRACTION_ONE,
				x_rotate * FRACTION_ONE,
				&ext_unit_x, &ext_unit_y);
    runit_x = (co_aix)ext_unit_x / FRACTION_ONE;
    runit_y = (co_aix)ext_unit_y / FRACTION_ONE;
    CU_ASSERT((runit_x / unit_x) < 1.01 &&
	      (runit_x / unit_x) > 0.99);
    CU_ASSERT((runit_y / unit_y) < 1.01 &&
	      (runit_y / unit_y) > 0.99);
}

void test_get_center_ref_shift(void) {
    co_aix slope;
    int slope_index;
    co_aix arc_len;
    co_aix arc_x, arc_y;
    int large, sweep;
    co_aix shift_x, shift_y;
    co_aix r_x, r_y;
    int64_t rshift_x, rshift_y;

    arc_x = 311;
    arc_y = 210;
    large = 0;			/* small arc */
    sweep = 1;			/* positive sweep */
    arc_len = sqrt(arc_x * arc_x + arc_y * arc_y);
    shift_x = arc_y / arc_len * (1 << REF_RADIUS_SHIFT);
    shift_y = arc_x / arc_len * (1 << REF_RADIUS_SHIFT);
    if((arc_x < 0) ^ (arc_y < 0))
	/* exactly one of arc_x and arc_y is negative */
	shift_y = -shift_y;
    else
	shift_x = -shift_x;
    slope = MIN(ABS(arc_x), ABS(arc_y)) /  MAX(ABS(arc_x), ABS(arc_y));
    slope_index = _find_slope_index(slope * FRACTION_ONE);
    _get_center_ref_shift(arc_x * FRACTION_ONE, arc_y * FRACTION_ONE,
			  large, sweep,
			  slope_index,
			  &rshift_x, &rshift_y);
    r_x = (co_aix)rshift_x / FRACTION_ONE;
    r_y = (co_aix)rshift_y / FRACTION_ONE;
    CU_ASSERT((r_x / shift_x) < 1.01 &&
	      (r_x / shift_x) > 0.99);
    CU_ASSERT((r_y / shift_y) < 1.01 &&
	      (r_y / shift_y) > 0.99);

    arc_x = 311;
    arc_y = 210;
    large = 1;			/* small arc */
    sweep = 1;			/* positive sweep */
    arc_len = sqrt(arc_x * arc_x + arc_y * arc_y);
    shift_x = -arc_y / arc_len * (1 << REF_RADIUS_SHIFT);
    shift_y = -arc_x / arc_len * (1 << REF_RADIUS_SHIFT);
    if((arc_x < 0) ^ (arc_y < 0))
	/* exactly one of arc_x and arc_y is negative */
	shift_y = -shift_y;
    else
	shift_x = -shift_x;
    slope = MIN(ABS(arc_x), ABS(arc_y)) /  MAX(ABS(arc_x), ABS(arc_y));
    slope_index = _find_slope_index(slope * FRACTION_ONE);
    _get_center_ref_shift(arc_x * FRACTION_ONE, arc_y * FRACTION_ONE,
			  large, sweep,
			  slope_index,
			  &rshift_x, &rshift_y);
    r_x = (co_aix)rshift_x / FRACTION_ONE;
    r_y = (co_aix)rshift_y / FRACTION_ONE;
    CU_ASSERT((r_x / shift_x) < 1.01 &&
	      (r_x / shift_x) > 0.99);
    CU_ASSERT((r_y / shift_y) < 1.01 &&
	      (r_y / shift_y) > 0.99);
}

void test_calc_center(void) {
    co_aix x0, y0, x, y;
    co_aix rx, ry, x_rotate;
    int large, sweep;
    co_aix cx, cy;
    co_aix angle_start, angle_stop;
    co_aix rcx, rcy;
    co_aix _x, _y;
    
#define ELLIPSE_POINT(angle, point_x, point_y)			\
    do {							\
	_x = rx * cos(angle);					\
	_y = ry * sin(angle);					\
	point_x = _x * cos(x_rotate) - _y * sin(x_rotate) + cx;	\
	point_y = _x * sin(x_rotate) + _y * cos(x_rotate) + cy;	\
    } while(0)
#define CENTER_TEST()						\
    do {							\
	_calc_center(x0, y0, x, y, rx, ry, x_rotate,		\
		     0, 1, &rcx, &rcy);				\
	CU_ASSERT((cx - rcx) <= 2 && (cx - rcx) >= -2);		\
	CU_ASSERT((cy - rcy) <= 2 && (cy - rcy) >= -2);		\
	_calc_center(x0, y0, x, y, rx, ry, x_rotate,		\
		     1, 0, &rcx, &rcy);				\
	CU_ASSERT((cx - rcx) <= 2 && (cx - rcx) >= -2);		\
	CU_ASSERT((cy - rcy) <= 2 && (cy - rcy) >= -2);		\
	_calc_center(x, y, x0, y0, rx, ry, x_rotate,		\
		     0, 0, &rcx, &rcy);				\
	CU_ASSERT((cx - rcx) <= 2 && (cx - rcx) >= -2);		\
	CU_ASSERT((cy - rcy) <= 2 && (cy - rcy) >= -2);		\
	_calc_center(x, y, x0, y0, rx, ry, x_rotate,		\
		     1, 1, &rcx, &rcy);				\
	CU_ASSERT((cx - rcx) <= 2 && (cx - rcx) >= -2);		\
	CU_ASSERT((cy - rcy) <= 2 && (cy - rcy) >= -2);		\
    } while(0)

    cx = 135;
    cy = 254;
    rx = 200;
    ry = 170;
    x_rotate = PI * 20 / 180;
    angle_start = PI * 55 / 180;
    angle_stop = PI * 97 / 180;
    
    ELLIPSE_POINT(angle_start, x0, y0);
    ELLIPSE_POINT(angle_stop, x, y);
    CENTER_TEST();
}

void test_spaces_head_tail(void) {
    sh_path_t *path;
    redraw_man_t rdman;

    path = (sh_path_t *)
	rdman_shape_path_new(&rdman,
			     " M 33 25l33 55C 33 87 44 22 55 99L33 77z ");
    CU_ASSERT(path != NULL);
    sh_path_free((shape_t *)path);
}

CU_pSuite get_shape_path_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_shape_path", NULL, NULL);
    CU_ADD_TEST(suite, test_rdman_shape_path_new);
    CU_ADD_TEST(suite, test_path_transform);
    CU_ADD_TEST(suite, test_small_slope);
    CU_ADD_TEST(suite, test_find_slope_index);
    CU_ADD_TEST(suite, test_vector_len);
    CU_ADD_TEST(suite, test_find_arc_radius);
    CU_ADD_TEST(suite, test_get_arc_radius_shift_factor);
    CU_ADD_TEST(suite, test_compute_extend_unit_vector);
    CU_ADD_TEST(suite, test_get_center_ref_shift);
    CU_ADD_TEST(suite, test_calc_center);

    return suite;
}

#endif /* UNITTEST */
