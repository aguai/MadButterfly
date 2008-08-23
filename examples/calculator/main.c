#include <stdio.h>
#include <mb_types.h>
#include <X_supp.h>
#include <shapes.h>
#include <tools.h>
#include "calculator_scr.h"

typedef struct _ex_rt ex_rt_t;
struct _ex_rt {
    X_MB_runtime_t *rt;
    calculator_scr_t *code;
};

static struct {
    int c;
    int off;
} tgt_list[] = {
    { 0, OFFSET(calculator_scr_t, but_0) },
    { 1, OFFSET(calculator_scr_t, but_1) },
    { 2, OFFSET(calculator_scr_t, but_2) },
    { 3, OFFSET(calculator_scr_t, but_3) },
    { 4, OFFSET(calculator_scr_t, but_4) },
    { 5, OFFSET(calculator_scr_t, but_5) },
    { 6, OFFSET(calculator_scr_t, but_6) },
    { 7, OFFSET(calculator_scr_t, but_7) },
    { 8, OFFSET(calculator_scr_t, but_8) },
    { 9, OFFSET(calculator_scr_t, but_9) },
    { '+', OFFSET(calculator_scr_t, but_add) },
    { '-', OFFSET(calculator_scr_t, but_minus) },
    { '*', OFFSET(calculator_scr_t, but_mul) },
    { '/', OFFSET(calculator_scr_t, but_div) },
    { '=', OFFSET(calculator_scr_t, but_eq) },
    { 'c', OFFSET(calculator_scr_t, but_clr) }
};

static int real_compute(int op, int v1, int v2) {
    int r = v2;

    switch(op) {
    case '+':
	r = v1 + v2;
	break;
    case '-':
	r = v1 - v2;
	break;
    case '*':
	r = v1 * v2;
	break;
    case '/':
	if(v2)
	    r = v1 / v2;
	else
	    r = v1;
	break;
    }

    return r;
}

static void show_text(ex_rt_t *ex_rt, int num, const char *prefix) {
    char buf[20];

    sprintf(buf, "%s%d", prefix, num);
    sh_text_set_text(ex_rt->code->screen_text, buf);
    rdman_shape_changed(ex_rt->rt->rdman, ex_rt->code->screen_text);
}

static void compute(ex_rt_t *ex_rt, coord_t *tgt) {
    int i;
    coord_t **coord_p;
    static int num = 0;
    static int saved = 0, saved2 = 0;
    static int op = 0, op1 = 0;

    for(i = 0; i < 16; i++) {
	coord_p = (coord_t **)((void *)ex_rt->code + tgt_list[i].off);
	if(*coord_p == (void *)tgt)
	    break;
    }
    if(i >= 16) return;

    if(i < 10) {
	num = num * 10 + i;
	show_text(ex_rt, num, "");
    } else {
	switch(tgt_list[i].c) {
	case 'c':
	    saved = num = 0;
	    show_text(ex_rt, 0, "");
	    break;

	case '+':
	case '-':
	case '*':
	case '/':
	    saved = real_compute(op, saved, num);
	    show_text(ex_rt, saved, "=");
	    op = tgt_list[i].c;
	    saved2 = num;
	    num = 0;
	    break;

	case '=':
	    saved = real_compute(op, saved, saved2);
	    show_text(ex_rt, saved, "=");
	    num = 0;
	    break;
	}
    }
    rdman_redraw_changed(ex_rt->rt->rdman);
}

static void buttons_handler(event_t *evt, void *arg) {
    ex_rt_t *ex_rt = (ex_rt_t *)arg;

    switch(evt->type) {
    case EVT_MOUSE_BUT_PRESS:
	compute(ex_rt, (coord_t *)evt->cur_tgt);
	break;
    }
}

static void setup_observers(ex_rt_t *ex_rt) {
    calculator_scr_t *calculator_scr;
    ob_factory_t *factory;
    subject_t *subject;
    coord_t *coord;
    int off;
    int i;

    calculator_scr = ex_rt->code;
    factory = rdman_get_ob_factory(ex_rt->rt->rdman);

    for(i = 0; i < 16; i++) {
	off = tgt_list[i].off;
	coord = *(coord_t **)((void *)calculator_scr + off);
	subject = coord_get_mouse_event(coord);
	subject_add_observer(factory, subject, buttons_handler, ex_rt);
    }
}

int main(int argc, char * const argv[]) {
    X_MB_runtime_t rt;
    calculator_scr_t *calculator_scr;
    ex_rt_t ex_rt;
    int r;

    r = X_MB_init(":0.0", 300, 400, &rt);

    calculator_scr = calculator_scr_new(rt.rdman);

    ex_rt.rt = &rt;
    ex_rt.code = calculator_scr;
    setup_observers(&ex_rt);

    X_MB_handle_connection(&rt);

    calculator_scr_free(calculator_scr);
    X_MB_destroy(&rt);

    return 0;
}
