#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <cairo.h>
#include "mb_types.h"

typedef struct _sh_path {
    shape_t shape;
    int cmd_len;
    int arg_len;
    char *user_data;
    char *dev_data;		/* device space data */
} sh_path_t;

#define ASSERT(x)
#define SKIP_SPACE(x) while(*(x) && isspace(*(x))) { (x)++; }
#define SKIP_NUM(x)					\
    while(*(x) &&					\
	  (isdigit(*(x)) ||				\
	   *(x) == '-' ||				\
	   *(x) == '+'))  {				\
	(x)++;						\
    }
#define OK 0
#define ERR -1

void sh_path_free(shape_t *shape) {
    sh_path_t *path = (sh_path_t *)shape;
    if(path->user_data)
	free(path->user_data);
    if(path->dev_data)
	free(path->dev_data);
    free(path);
}

static int sh_path_cmd_arg_cnt(char *data, int *cmd_cntp, int *arg_cntp) {
    char *p, *old;
    int cmd_cnt, arg_cnt;

    cmd_cnt = arg_cnt = 0;
    p = data;
    while(*p) {
	SKIP_SPACE(p);

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
	    }
	    break;
	case 'z':
	case 'Z':
	    break;
	default:
	    return ERR;
	}
	cmd_cnt++;
    }

    *cmd_cntp = cmd_cnt;
    *arg_cntp = arg_cnt;
    return OK;
}

static int sh_path_cmd_arg_fill(char *data, sh_path_t *path) {
    char *p, *old;
    char *cmds;
    co_aix *args;

    cmds = path->user_data;
    args = (co_aix *)(cmds + path->cmd_len);
    p = data;
    while(*p) {
	SKIP_SPACE(p);

	/* TODO: transform all relative to absolute, */
	*cmds++ = *p;
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
		*args++ = atof(old);

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args++ = atof(old);

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args++ = atof(old);

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args++ = atof(old);
		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args++ = atof(old);

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args++ = atof(old);
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
		*args++ = atof(old);

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args++ = atof(old);

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args++ = atof(old);

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args++ = atof(old);
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
		*args++ = atof(old);

		SKIP_SPACE(p);
		old = p;
		SKIP_NUM(p);
		if(p == old)
		    return ERR;
		*args++ = atof(old);
	    }
	    break;
	case 'h':
	case 'H':
	case 'v':
	case 'V':
	    /* TODO: implement h, H, v, V comamnds for path. */
	    return ERR;
	case 'z':
	case 'Z':
	    break;
	default:
	    return ERR;
	}
    }

    return OK;
}

shape_t *sh_path_new(char *data) {
    sh_path_t *path;
    int cmd_cnt, arg_cnt;
    int r;

    r = sh_path_cmd_arg_cnt(data, &cmd_cnt, &arg_cnt);
    if(r == ERR)
	return NULL;

    cmd_cnt = (cmd_cnt + 3) & ~0x3;

    path = (sh_path_t *)malloc(sizeof(sh_path_t));
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

    return (shape_t *)path;
}

void sh_path_transform(shape_t *shape, coord_t *coord) {
    sh_path_t *path;
    co_aix *user_args, *dev_args;
    int i;

    ASSERT(shape->type == SHT_PATH);
    ASSERT((shape->arg_len & 0x1) == 0);

    path = (sh_path_t *)shape;
    user_args = (co_aix *)(path->user_data + path->cmd_len);
    dev_args = (co_aix *)(path->dev_data + path->cmd_len);
    for(i = 0; i < path->arg_len; i += 2) {
	dev_args[0] = *user_args++;
	dev_args[1] = *user_args++;
	coord_trans_pos(coord, dev_args, dev_args + 1);
	dev_args += 2;
    }
}

void sh_path_draw(shape_t *shape, cairo_t *cr) {
    sh_path_t *path;
    int cmd_len;
    char *cmds, cmd;
    co_aix *args;
    co_aix x, y, x1, y1, x2, y2;
    int i;

    ASSERT(shape->type == SHT_PATH);

    path = (sh_path_t *)shape;
    cmd_len = path->cmd_len;
    cmds = path->dev_data;
    args = (co_aix *)(cmds + cmd_len);
    x = y = 0;
    for(i = 0; i < cmd_len; i++) {
	cmd = *cmds++;
	switch(cmd) {
	case 'm':
	    x = x + *args++;
	    y = y + *args++;
	    cairo_move_to(cr, x, y);
	    break;
	case 'M':
	    x = *args++;
	    y = *args++;
	    cairo_move_to(cr, x, y);
	    break;
	case 'l':
	    x = x + *args++;
	    y = y + *args++;
	    cairo_line_to(cr, x, y);
	    break;
	case 'L':
	    x = *args++;
	    y = *args++;
	    cairo_line_to(cr, x, y);
	    break;
	case 'c':
	    x1 = x + *args++;
	    y1 = y + *args++;
	    x2 = x1 + *args++;
	    y2 = y1 + *args++;
	    x = x2 + *args++;
	    y = y2 + *args++;
	    cairo_curve_to(cr, x1, y1, x2, y2, x, y);
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
	case 's':
	    x1 = x + x - x2;
	    y1 = y + y - y2;
	    x2 = x1 + *args++;
	    y2 = y1 + *args++;
	    x = x2 + *args++;
	    y = y2 + *args++;
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
	case 'q':
	    x1 = x + *args++;
	    y1 = y + *args++;
	    x2 = x1;
	    y2 = y1;
	    x = x2 + *args++;
	    y = y2 + *args++;
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
	case 't':
	    x1 = x + x - x2;
	    y1 = y + y - y2;
	    x2 = x1;
	    y2 = y1;
	    x = x2 + *args++;
	    y = y2 + *args++;
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
	case 'Z':
	case 'z':
	    cairo_close_path(cr);
	    break;
	case '\x0':
	    break;
	}
    }

    cairo_fill(cr);
}

#ifdef UNITTEST

#include <CUnit/Basic.h>

void test_sh_path_new(void) {
    sh_path_t *path;
    co_aix *args;

    path = (sh_path_t *)sh_path_new("M 33 25l33 55C 33 87 44 22 55 99L33 77z");
    CU_ASSERT(path != NULL);
    CU_ASSERT(path->cmd_len == 8);
    CU_ASSERT(path->arg_len == 12);
    CU_ASSERT(strcmp(path->user_data, "MlCLz") == 0);
    CU_ASSERT(strcmp(path->dev_data, "MlCLz") == 0);

    args = (co_aix *)(path->user_data + path->cmd_len);
    CU_ASSERT(args[0] == 33);
    CU_ASSERT(args[1] == 25);
    CU_ASSERT(args[2] == 33);
    CU_ASSERT(args[3] == 55);
    CU_ASSERT(args[4] == 33);
    CU_ASSERT(args[5] == 87);
    CU_ASSERT(args[6] == 44);
    CU_ASSERT(args[7] == 22);
    CU_ASSERT(args[8] == 55);
    CU_ASSERT(args[9] == 99);
    CU_ASSERT(args[10] == 33);
    CU_ASSERT(args[11] == 77);
    sh_path_free((shape_t *)path);
}

void test_path_transform(void) {
    sh_path_t *path;
    co_aix *args;
    coord_t coord;

    path = (sh_path_t *)sh_path_new("M 33 25l33 55C 33 87 44 22 55 99L33 77z");
    CU_ASSERT(path != NULL);
    CU_ASSERT(path->cmd_len == 8);
    CU_ASSERT(path->arg_len == 12);
    CU_ASSERT(strcmp(path->user_data, "MlCLz") == 0);
    CU_ASSERT(strcmp(path->dev_data, "MlCLz") == 0);

    coord.aggr_matrix[0] = 1;
    coord.aggr_matrix[1] = 0;
    coord.aggr_matrix[2] = 1;
    coord.aggr_matrix[3] = 0;
    coord.aggr_matrix[4] = 2;
    coord.aggr_matrix[5] = 0;
    sh_path_transform((shape_t *)path, &coord);

    args = (co_aix *)(path->dev_data + path->cmd_len);
    CU_ASSERT(args[0] == 34);
    CU_ASSERT(args[1] == 50);
    CU_ASSERT(args[2] == 34);
    CU_ASSERT(args[3] == 110);
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

CU_pSuite get_shape_path_suite(void) {
    CU_pSuite suite;

    suite = CU_add_suite("Suite_shape_path", NULL, NULL);
    CU_ADD_TEST(suite, test_sh_path_new);
    CU_ADD_TEST(suite, test_path_transform);

    return suite;
}

#endif /* UNITTEST */
