#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <cairo.h>
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
    int arg_len;
    int fix_arg_len;
    char *user_data;
    char *dev_data;		/* device space data */
} sh_path_t;
#define RESERVED_AIXS sizeof(co_aix[2])

#define ASSERT(x)
#define SKIP_SPACE(x) while(*(x) && (isspace(*(x)) || *(x) == ',')) { (x)++; }
#define SKIP_NUM(x)					\
    while(*(x) &&					\
	  (isdigit(*(x)) ||				\
	   *(x) == 'e' ||				\
	   *(x) == 'E' ||				\
	   *(x) == '-' ||				\
	   *(x) == '+' ||				\
	   *(x) == '.'))  {					\
	(x)++;						\
    }
#define OK 0
#define ERR -1
#define PI 3.1415926

/* ============================================================
 * Implement arc in path.
 */
#include <math.h>
/*! \brief Calculate center of the ellipse of an arc.
 *
 * - ux0 = x0 / rx
 * - uy0 = y0 / ry
 * - ux = x / rx
 * - uy = y / rx
 * ux0, uy0, ux, yu are got by transforming (x0, y0) and (x, y) into points
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
 * - udl2 = udx ** 2 + udy ** 2;
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
static int calc_center_and_x_aix(co_aix x0, co_aix y0,
				 co_aix x, co_aix y,
				 co_aix rx, co_aix ry,
				 co_aix x_rotate,
				 int large, int sweep,
				 co_aix *cx, co_aix *cy,
				 co_aix *xx, co_aix *xy) {
    co_aix nrx, nry, nrx0, nry0;
    co_aix udx, udy, udx2, udy2;
    co_aix umx, umy;
    co_aix udcx, udcy;
    co_aix nrcx, nrcy;
    co_aix udl2;
    float _sin = sinf(x_rotate);
    float _cos = cosf(x_rotate);
    int reflect;
    
    nrx = x * _cos + y * _sin;
    nry = x * -_sin + y * _cos;
    nrx0 = x0 * _cos + y0 * _sin;
    nry0 = x0 * -_sin + y0 * _cos;
    
    udx = (nrx - nrx0) / 2 / rx; /* ux - umx */
    udy = (nry - nry0) / 2 / ry; /* uy - umy */
    umx = (nrx + nrx0) / 2 / rx;
    umy = (nry + nry0) / 2 / ry;

    udx2 = udx * udx;
    udy2 = udy * udy;
    udl2 = udx2 + udy2;

    if(udy != 0) {
	udcx = -sqrtf((1 - udl2) * udl2) / (udy + udx2 / udy);
	udcy = -udcx * udx / udy;
    } else {
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

    nrcx = rx * (udcx + umx);
    nrcy = ry * (udcy + umy);

    *cx = nrcx * _cos - nrcy * _sin;
    *cy = nrcx * _sin + nrcy * _cos;

    *xx = rx * _cos + *cx;
    *xy = rx * _sin + *cy;

    return OK;
}


#define TAKE_NUM(r) do {			\
	SKIP_SPACE(p);				\
	old = p;				\
	SKIP_NUM(p);				\
	if(p == old)				\
	    return ERR;				\
	r = atof(old);				\
    } while(0);

static int sh_path_arc_cmd_arg_fill(char cmd, char **cmds_p,
				    const char **data_p,
				    co_aix **args_p,
				    int **fix_args_p) {
    co_aix rx, ry;
    co_aix x_rotate;
    int large, sweep;
    co_aix x, y, x0, y0, cx, cy, xx, xy;
    co_aix *args = *args_p;
    const char *old;
    const char *p;
    char *cmds;
    int *fix_args;

    p = *data_p;
    cmds = *cmds_p;
    fix_args = *fix_args_p;
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

	x0 = *(args - 2);
	y0 = *(args - 1);

	if(islower(cmd)) {
	    x += x0;
	    y += y0;
	}

	calc_center_and_x_aix(x0, y0, x, y,
			      rx, ry,
			      x_rotate, large, sweep,
			      &cx, &cy, &xx, &xy);

	*(args++) = cx;
	*(args++) = cy;
	*(args++) = xx;
	*(args++) = xy;
	*(args++) = x;
	*(args++) = y;

	*cmds++ = toupper(cmd);
	*fix_args++ = sweep;
    }

    *data_p = p;
    *args_p = args;
    *cmds_p = cmds;
    *fix_args_p = fix_args;

    return OK;
}

#define INNER(x1, y1, x2, y2) ((x1) * (x2) + (y1) * (y2))
#define CROSS(x1, y1, x2, y2) ((x1) * (y2) - (y1) * (x2))

/*! \brief Make path for arcs in a path.
 */
void sh_path_arc_path(cairo_t *cr, const co_aix **args_p,
		      const int **fix_args_p) {
    co_aix cx, cy, x0, y0, x, y, xx, xy;
    co_aix dx, dy, dx0, dy0, dxx, dxy;
    co_aix xyratio;
    co_aix rx;
    co_aix rx2;
    co_aix inner0, cross0;
    co_aix circle_h0;
    co_aix inner, cross;
    co_aix angle, angle0;
    co_aix rotate;
    const co_aix *args = *args_p;
    const int *fix_args = *fix_args_p;
    int sweep;

    x0 = *(args - 2);
    y0 = *(args - 1);
    cx = *args++;
    cy = *args++;
    xx = *args++;
    xy = *args++;
    x = *args++;
    y = *args++;
    sweep = *fix_args++;

    dx = x - cx;
    dy = y - cy;
    dx0 = x0 - cx;
    dy0 = y0 - cy;
    dxx = xx - cx;
    dxy = xy - cy;

    rx2 = dxx * dxx + dxy * dxy;
    rx = sqrtf(rx2);

    /*! \note  Why we calculate these numbers there?
     * If we compute it when filling arguments, sh_path_arc_cmd_arg_fill(),
     * we can avoid to recompute it for every drawing.  But, transforming of
     * coordinate can effect value of the numbers.
     */
    inner0 = INNER(dxx, dxy, dx0, dy0);
    cross0 = CROSS(dxx, dxy, dx0, dy0);
    circle_h0 = sqrtf(rx2 - inner0 * inner0 / rx2);
    xyratio = cross0 / rx / circle_h0;
    if(xyratio < 0)
	xyratio = -xyratio;

    angle0 = acos(inner0 / rx2);
    if(cross0 < 0)
	angle0 = PI * 2 - angle0; /* 3rd, 4th Quadrant */

    inner = INNER(dxx, dxy, dx, dy);
    cross = CROSS(dxx, dxy, dx, dy);
    angle = acos(inner / rx2);
    if(cross < 0)
	angle = PI * 2 - angle; /* 3rd, 4th Quadrant */

    /* Make a path for arc */
    rotate = acos(dxx / rx);
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_rotate(cr, rotate);
    cairo_scale(cr, 1.0, xyratio);
    if(sweep)
	cairo_arc(cr, 0, 0, rx, angle0, angle);
    else
	cairo_arc_negative(cr, 0, 0, rx, angle0, angle);
    cairo_restore(cr);

    *args_p = args;
    *fix_args_p = fix_args;
}

/* ============================================================ */

static void sh_path_free(shape_t *shape) {
    sh_path_t *path = (sh_path_t *)shape;
    if(path->user_data)
	free(path->user_data);
    free(path);
}

/*! \brief Count number of arguments.
 *
 * \todo Notify programmers that syntax or value error of path data.
 */
static int sh_path_cmd_arg_cnt(char *data, int *cmd_cntp, int *arg_cntp,
			       int *fix_arg_cntp) {
    char *p, *old;
    int cmd_cnt, arg_cnt, fix_arg_cnt;
    int i;

    cmd_cnt = arg_cnt = fix_arg_cnt = 0;
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
		arg_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		arg_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		arg_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		arg_cnt++;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		arg_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		arg_cnt++;

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
		arg_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		arg_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		arg_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		arg_cnt++;

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
		arg_cnt++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		arg_cnt++;

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
		arg_cnt += 2;

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

		arg_cnt += 6;
		fix_arg_cnt++;

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
    *arg_cntp = arg_cnt;
    *fix_arg_cntp = fix_arg_cnt;
    return OK;
}

#define TO_ABSX islower(cmd)? x + atof(old): atof(old)
#define TO_ABSY islower(cmd)? y + atof(old): atof(old)

static int sh_path_cmd_arg_fill(char *data, sh_path_t *path) {
    char *p, *old;
    char *cmds;
    char cmd;
    co_aix *args;
    int *fix_args;
    co_aix x, y;
    int r;

    cmds = path->user_data;
    args = (co_aix *)(cmds + path->cmd_len);
    fix_args = (int *)(cmds + path->cmd_len +
		       path->arg_len * sizeof(co_aix));
    p = data;
    SKIP_SPACE(p);
    while(*p) {
	/* Transform all relative to absolute, */
	x = *(args - 2);
	y = *(args - 1);

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
		*args = TO_ABSX;
		args++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args = TO_ABSY;
		args++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args = TO_ABSX;
		args++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args = TO_ABSY;
		args++;
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args = TO_ABSX;
		args++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args = TO_ABSY;
		args++;

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
		*args = TO_ABSX;
		args++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args = TO_ABSY;
		args++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args = TO_ABSX;
		args++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args = TO_ABSY;
		args++;

		*cmds++ = toupper(cmd);
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
		*args = TO_ABSX;
		args++;

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args = TO_ABSY;
		args++;

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
	    r = sh_path_arc_cmd_arg_fill(cmd, &cmds,
					 (const char **)&p, &args,
					 &fix_args);
	    if(r != OK)
		return ERR;
	    break;

	case 'z':
	case 'Z':
	    *cmds++ = toupper(cmd);
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
shape_t *rdman_shape_path_new(redraw_man_t *rdman, char *data) {
    sh_path_t *path;
    int cmd_cnt, arg_cnt, fix_arg_cnt;
    int msz;
    int r;

    r = sh_path_cmd_arg_cnt(data, &cmd_cnt, &arg_cnt, &fix_arg_cnt);
    if(r == ERR)
	return NULL;

    /* Align at 4's boundary and keep 2 unused co_aix space
     * to make logic of transformation from relative to absolute
     * simple.
     */
    cmd_cnt += RESERVED_AIXS;
    cmd_cnt = (cmd_cnt + 3) & ~0x3;

    /*! \todo Use elmpool to manage sh_path_t objects. */
    path = (sh_path_t *)malloc(sizeof(sh_path_t));
    /*! \todo Remove this memset()? */
    memset(&path->shape, 0, sizeof(shape_t));
    mb_obj_init(path, MBO_PATH);
    path->cmd_len = cmd_cnt;
    path->arg_len = arg_cnt;
    path->fix_arg_len = fix_arg_cnt;

    msz = cmd_cnt + sizeof(co_aix) * arg_cnt + sizeof(int) * fix_arg_cnt;
    path->user_data = (char *)malloc(msz * 2);
    if(path->user_data == NULL) {
	free(path);
	return NULL;
    }

    path->dev_data = path->user_data + msz;

    r = sh_path_cmd_arg_fill(data, path);
    if(r == ERR) {
	free(path->user_data);
	free(path);
	return NULL;
    }
    memcpy(path->dev_data, path->user_data, msz);

    path->shape.free = sh_path_free;

#ifndef UNITTEST
    rdman_shape_man(rdman, (shape_t *)path);
#endif

    return (shape_t *)path;
}

shape_t *rdman_shape_path_new_from_binary(redraw_man_t *rdman, char *commands, co_aix *arg,int  arg_cnt,int *fix_arg,int fix_arg_cnt) {
    sh_path_t *path;
    int msz;
    int cmd_cnt = strlen(commands);

    /*! \todo Use elmpool to manage sh_path_t objects. */
    path = (sh_path_t *)malloc(sizeof(sh_path_t));
    /*! \todo Remove this memset()? */
    memset(&path->shape, 0, sizeof(shape_t));
    mb_obj_init(path, MBO_PATH);
    path->cmd_len = strlen(commands);
    path->arg_len = arg_cnt;
    path->fix_arg_len = fix_arg_cnt;
    msz = cmd_cnt + sizeof(co_aix) * arg_cnt + sizeof(int) * fix_arg_cnt;
    path->user_data = (char *)malloc(msz * 2);
    if(path->user_data == NULL) {
	free(path);
	return NULL;
    }

    path->dev_data = path->user_data + msz;
    memcpy(path->user_data,commands,cmd_cnt);
    memcpy(path->user_data+cmd_cnt,arg, sizeof(co_aix)*arg_cnt);
    memcpy(path->user_data+cmd_cnt+arg_cnt*sizeof(co_aix),fix_arg, sizeof(int)*fix_arg_cnt);
    memcpy(path->dev_data, path->user_data, msz);

    path->shape.free = sh_path_free;

#ifndef UNITTEST
    rdman_shape_man(rdman, (shape_t *)path);
#endif

    return (shape_t *)path;
}


/*! \brief Transform a path from user space to device space.
 *
 */
void sh_path_transform(shape_t *shape) {
    sh_path_t *path;
    co_aix *user_args, *dev_args;
    co_aix (*poses)[2];
    area_t *area;
    int arg_len;
    int i;

    ASSERT(shape->type == SHT_PATH);
    ASSERT((shape->arg_len & 0x1) == 0);

    path = (sh_path_t *)shape;
    user_args = (co_aix *)(path->user_data + path->cmd_len);
    dev_args = (co_aix *)(path->dev_data + path->cmd_len);
    arg_len = path->arg_len;
    for(i = 0; i < arg_len; i += 2) {
	dev_args[0] = *user_args++;
	dev_args[1] = *user_args++;
	coord_trans_pos(shape->coord, dev_args, dev_args + 1);
	dev_args += 2;
    }

    if(path->shape.geo) {
	poses = (co_aix (*)[2])(path->dev_data + path->cmd_len);
	geo_from_positions(path->shape.geo, arg_len / 2, poses);
	area = shape->geo->cur_area;
	area->x -= shape->stroke_width/2 + 1;
	area->y -= shape->stroke_width/2 + 1;
	area->w += shape->stroke_width + 2;
	area->h += shape->stroke_width + 2;
    }
}

static void sh_path_path(shape_t *shape, cairo_t *cr) {
    sh_path_t *path;
    int cmd_len;
    char *cmds, cmd;
    const co_aix *args;
    const int *fix_args;
    co_aix x, y, x1, y1, x2, y2;
    int i;

    ASSERT(shape->type == SHT_PATH);

    path = (sh_path_t *)shape;
    cmd_len = path->cmd_len;
    cmds = path->dev_data;
    args = (co_aix *)(cmds + cmd_len);
    fix_args = (int *)(cmds + cmd_len + path->arg_len * sizeof(co_aix));
    x = y = x1 = y1 = x2 = y2 = 0;
    for(i = 0; i < cmd_len; i++) {
	/* All path commands and arguments are transformed
	 * to absoluted form.
	 */
	cmd = *cmds++;
	switch(cmd) {
	case 'M':
	    x = *args++;
	    y = *args++;
	    cairo_move_to(cr, x, y);
	    break;
	case 'L':
	    x = *args++;
	    y = *args++;
	    cairo_line_to(cr, x, y);
	    break;
	case 'C':
	    x1 = *args++;
	    y1 = *args++;
	    x2 = *args++;
	    y2 = *args++;
	    x = *args++;
	    y = *args++;
	    cairo_curve_to(cr, x1, y1, x2, y2, x, y);
	    break;
	case 'S':
	    x1 = x + x - x2;
	    y1 = y + y - y2;
	    x2 = *args++;
	    y2 = *args++;
	    x = *args++;
	    y = *args++;
	    cairo_curve_to(cr, x1, y1, x2, y2, x, y);
	    break;
	case 'Q':
	    x1 = *args++;
	    y1 = *args++;
	    x2 = x1;
	    y2 = y1;
	    x = *args++;
	    y = *args++;
	    cairo_curve_to(cr, x1, y1, x2, y2, x, y);
	    break;
	case 'T':
	    x1 = x + x - x2;
	    y1 = y + y - y2;
	    x2 = x1;
	    y2 = y1;
	    x = *args++;
	    y = *args++;
	    cairo_curve_to(cr, x1, y1, x2, y2, x, y);
	    break;
	case 'A':
	    sh_path_arc_path(cr, &args, &fix_args);
	    break;
	case 'Z':
	    cairo_close_path(cr);
	    break;
	case '\x0':
	    i = cmd_len;	/* padding! Skip remain ones. */
	    break;
	}
    }
}

void sh_path_draw(shape_t *shape, cairo_t *cr) {
    sh_path_path(shape, cr);
}

#ifdef UNITTEST

#include <CUnit/Basic.h>

void test_rdman_shape_path_new(void) {
    sh_path_t *path;
    co_aix *args;

    path = (sh_path_t *)rdman_shape_path_new(NULL, "M 33 25l33 55c 33 87 44 22 55 99L33 77z");
    CU_ASSERT(path != NULL);
    CU_ASSERT(path->cmd_len == ((5 + RESERVED_AIXS + 3) & ~0x3));
    CU_ASSERT(path->arg_len == 12);
    CU_ASSERT(strncmp(path->user_data, "MLCLZ", 5) == 0);
    CU_ASSERT(strncmp(path->dev_data, "MLCLZ", 5) == 0);

    args = (co_aix *)(path->user_data + path->cmd_len);
    CU_ASSERT(args[0] == 33);
    CU_ASSERT(args[1] == 25);
    CU_ASSERT(args[2] == 66);
    CU_ASSERT(args[3] == 80);
    CU_ASSERT(args[4] == 99);
    CU_ASSERT(args[5] == 167);
    CU_ASSERT(args[6] == 110);
    CU_ASSERT(args[7] == 102);
    CU_ASSERT(args[8] == 121);
    CU_ASSERT(args[9] == 179);
    CU_ASSERT(args[10] == 33);
    CU_ASSERT(args[11] == 77);
    sh_path_free((shape_t *)path);
}

void test_path_transform(void) {
    sh_path_t *path;
    co_aix *args;
    coord_t coord;
    geo_t geo;

    path = (sh_path_t *)rdman_shape_path_new(NULL, "M 33 25l33 55C 33 87 44 22 55 99L33 77z");
    CU_ASSERT(path != NULL);
    CU_ASSERT(path->cmd_len == ((5 + RESERVED_AIXS + 3) & ~0x3));
    CU_ASSERT(path->arg_len == 12);
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

    args = (co_aix *)(path->dev_data + path->cmd_len);
    CU_ASSERT(args[0] == 34);
    CU_ASSERT(args[1] == 50);
    CU_ASSERT(args[2] == 67);
    CU_ASSERT(args[3] == 160);
    CU_ASSERT(args[4] == 34);
    CU_ASSERT(args[5] == 174);
    CU_ASSERT(args[6] == 45);
    CU_ASSERT(args[7] == 44);
    CU_ASSERT(args[8] == 56);
    CU_ASSERT(args[9] == 198);
    CU_ASSERT(args[10] == 34);
    CU_ASSERT(args[11] == 154);

    sh_path_free((shape_t *)path);
}

void test_spaces_head_tail(void) {
    sh_path_t *path;

    path = (sh_path_t *)
	rdman_shape_path_new(NULL,
			     " M 33 25l33 55C 33 87 44 22 55 99L33 77z ");
    CU_ASSERT(path != NULL);
    sh_path_free((shape_t *)path);
}

CU_pSuite get_shape_path_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_shape_path", NULL, NULL);
    CU_ADD_TEST(suite, test_rdman_shape_path_new);
    CU_ADD_TEST(suite, test_path_transform);

    return suite;
}

#endif /* UNITTEST */
