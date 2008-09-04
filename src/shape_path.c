#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <cairo.h>
#include "mb_types.h"

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
    char *user_data;
    char *dev_data;		/* device space data */
} sh_path_t;
#define RESERVED_AIXS sizeof(co_aix[2])

#define ASSERT(x)
#define SKIP_SPACE(x) while(*(x) && (isspace(*(x)) || *(x) == ',')) { (x)++; }
#define SKIP_NUM(x)					\
    while(*(x) &&					\
	  (isdigit(*(x)) ||				\
	   *(x) == '-' ||				\
	   *(x) == '+' ||				\
	   *(x) == '.'))  {					\
	(x)++;						\
    }
#define OK 0
#define ERR -1

static void sh_path_free(shape_t *shape) {
    sh_path_t *path = (sh_path_t *)shape;
    if(path->user_data)
	free(path->user_data);
    if(path->dev_data)
	free(path->dev_data);
    free(path);
}

/*! \brief Count number of arguments.
 *
 * \todo Notify programmers that syntax or value error of path data.
 */
static int sh_path_cmd_arg_cnt(char *data, int *cmd_cntp, int *arg_cntp) {
    char *p, *old;
    int cmd_cnt, arg_cnt;
    int i;

    cmd_cnt = arg_cnt = 0;
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
    return OK;
}

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
 * - udcx * udx + udcy * udy = 0
 * - udcy = - udcx * udx / udy
 * - udcx ** 2 + udcy ** 2 + udx ** 2 + udy ** 2 = 1
 * - udcx ** 2 + (udcx * udx / udy) ** 2 = 1 - udx ** 2 - udy ** 2
 * - udcx ** 2 = (1 - udx ** 2 - udy ** 2) / (1 + (udx/udy) ** 2)
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
    float _sin = sinf(x_rotate);
    float _cos = cosf(x_rotate);
    
    nrx = x * _cos + y * _sin;
    nry = x * -_sin + y * _cos;
    nrx0 = x0 * _cos + y0 * _sin;
    nry0 = x0 * -_sin + y0 * _cos;

    udx = (nrx - nrx0) / 2 / rx;
    udy = (nry - nry0) / 2 / ry;
    umx = (nrx + nrx0) / 2 / rx;
    umy = (nry + nry0) / 2 / ry;
    udx2 = udx * udx;
    udy2 = udy * udy;
    udcx = sqrtf((1 - udx2 - udy2) / (1 + udx2 / udy2));
    udcy = - udcx * udx / udy;
    nrcx = rx * (udcx + umx);
    nrcy = ry * (udcy + umy);

    *cx = nrcx * _cos - nrcy * _sin;
    *cy = nrcx * _sin + nrcy * _cos;

    *xx = rx * _cos + *cx;
    *xy = rx * _sin + *cy;

    return OK;
}


static int sh_path_arc_cmd_arg_fill(char cmd, char **cmds_p,
				    const char **data_p, co_aix **args_p) {
    co_aix rx, ry;
    co_aix x_rotate;
    int large, sweep;
    co_aix x, y, x0, y0, cx, cy, xx, xy;
    co_aix *args = *args_p;
    const char *old;
    const char *p;
    char *cmds;

    p = *data_p;
    cmds = *cmds_p;
    while(*p) {
	SKIP_SPACE(p);
	old = p;
	SKIP_NUM(p);
	if(p == old)
	    break;
	rx = atof(old);

	SKIP_SPACE(p);
	old = p;
	SKIP_NUM(p);
	if(p == old)
	    return ERR;
	ry = atof(old);

	SKIP_SPACE(p);
	old = p;
	SKIP_NUM(p);
	if(p == old)
	    return ERR;
	x_rotate = atof(old);

	SKIP_SPACE(p);
	old = p;
	SKIP_NUM(p);
	if(p == old)
	    return ERR;
	large = atoi(old);
	
	SKIP_SPACE(p);
	old = p;
	SKIP_NUM(p);
	if(p == old)
	    return ERR;
	sweep = atoi(old);

	SKIP_SPACE(p);
	old = p;
	SKIP_NUM(p);
	if(p == old)
	    return ERR;
	x = atof(old);

	SKIP_SPACE(p);
	old = p;
	SKIP_NUM(p);
	if(p == old)
	    return ERR;
	y = atof(old);

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
    }

    *data_p = p;
    *args_p = args;
    *cmds_p = cmds;

    return OK;
}

void sh_path_arc_path(cairo_t *cr, const co_aix **args) {
    
}

#define TO_ABSX islower(cmd)? x + atof(old): atof(old)
#define TO_ABSY islower(cmd)? y + atof(old): atof(old)

static int sh_path_cmd_arg_fill(char *data, sh_path_t *path) {
    char *p, *old;
    char *cmds;
    char cmd;
    co_aix *args;
    co_aix x, y;
    int r;

    cmds = path->user_data;
    args = (co_aix *)(cmds + path->cmd_len);
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
	    r = sh_path_arc_cmd_arg_fill(cmd, &cmds, (const char **)&p, &args);
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
shape_t *sh_path_new(char *data) {
    sh_path_t *path;
    int cmd_cnt, arg_cnt;
    int r;

    r = sh_path_cmd_arg_cnt(data, &cmd_cnt, &arg_cnt);
    if(r == ERR)
	return NULL;

    /* Align at 4's boundary and keep 2 unused co_aix space
     * to make logic of transformation from relative to absolute
     * simple.
     */
    cmd_cnt += RESERVED_AIXS;
    cmd_cnt = (cmd_cnt + 3) & ~0x3;

    path = (sh_path_t *)malloc(sizeof(sh_path_t));
    memset(&path->shape, 0, sizeof(shape_t));
    path->shape.sh_type = SHT_PATH;
    path->cmd_len = cmd_cnt;
    path->arg_len = arg_cnt;
    path->user_data = (char *)malloc(cmd_cnt + sizeof(co_aix) * arg_cnt);
    if(path->user_data == NULL) {
	free(path);
	return NULL;
    }
    path->dev_data = (char *)malloc(cmd_cnt + sizeof(co_aix) * arg_cnt);
    if(path->dev_data == NULL) {
	free(path->dev_data);
	free(path);
	return NULL;
    }

    memset(path->user_data, 0, cmd_cnt);
    r = sh_path_cmd_arg_fill(data, path);
    if(r == ERR) {
	free(path->user_data);
	free(path->dev_data);
	free(path);
	return NULL;
    }
    memcpy(path->dev_data, path->user_data, cmd_cnt);

    path->shape.free = sh_path_free;

    return (shape_t *)path;
}

/*! \brief Transform a path from user space to device space.
 *
 * \todo associate coord_t with shape objects and transform them
 *       automatically.
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
    co_aix x, y, x1, y1, x2, y2;
    int i;

    ASSERT(shape->type == SHT_PATH);

    path = (sh_path_t *)shape;
    cmd_len = path->cmd_len;
    cmds = path->dev_data;
    args = (co_aix *)(cmds + cmd_len);
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
	    sh_path_arc_path(cr, &args);
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

void test_sh_path_new(void) {
    sh_path_t *path;
    co_aix *args;

    path = (sh_path_t *)sh_path_new("M 33 25l33 55c 33 87 44 22 55 99L33 77z");
    CU_ASSERT(path != NULL);
    CU_ASSERT(path->cmd_len == ((5 + RESERVED_AIXS + 3) & ~0x3));
    CU_ASSERT(path->arg_len == 12);
    CU_ASSERT(strcmp(path->user_data, "MLCLZ") == 0);
    CU_ASSERT(strcmp(path->dev_data, "MLCLZ") == 0);

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

    path = (sh_path_t *)sh_path_new("M 33 25l33 55C 33 87 44 22 55 99L33 77z");
    CU_ASSERT(path != NULL);
    CU_ASSERT(path->cmd_len == ((5 + RESERVED_AIXS + 3) & ~0x3));
    CU_ASSERT(path->arg_len == 12);
    CU_ASSERT(strcmp(path->user_data, "MLCLZ") == 0);
    CU_ASSERT(strcmp(path->dev_data, "MLCLZ") == 0);

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
	sh_path_new(" M 33 25l33 55C 33 87 44 22 55 99L33 77z ");
    CU_ASSERT(path != NULL);
    sh_path_free((shape_t *)path);
}

CU_pSuite get_shape_path_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_shape_path", NULL, NULL);
    CU_ADD_TEST(suite, test_sh_path_new);
    CU_ADD_TEST(suite, test_path_transform);

    return suite;
}

#endif /* UNITTEST */
