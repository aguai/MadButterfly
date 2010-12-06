#include <stdio.h>
#include <string.h>
#include <mb.h>
#include "calculator_scr.h"

typedef struct _calc_data calc_data_t;
struct _calc_data {
    mb_rt_t *rt;
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
    int r = v1;

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
	r = v1;
	if(v2)
	    r /= v2;
	break;
    case 'n':
	r = v2;
	break;
    case 'N':
	break;
    }

    return r;
}

static void show_text(calc_data_t *calc_data, int num, int saved, int op,
		      const char *suffix) {
    char buf[20];
    redraw_man_t *rdman;

    rdman = mb_runtime_rdman(calc_data->rt);

    sprintf(buf, "%d%s", num, suffix);
    sh_stext_set_text(calc_data->code->screen_text_u, buf);
    calc_data->code->screen_text_u_style_blks[0].n_chars = strlen(buf);
    sh_stext_set_style(calc_data->code->screen_text_u,
		       calc_data->code->screen_text_u_style_blks, 1);
    rdman_shape_changed(rdman, calc_data->code->screen_text_u);

    if(op == 'n')
	sprintf(buf, "None");
    else
	sprintf(buf, "%d%c", saved, op);
    sh_stext_set_text(calc_data->code->saved_text, buf);
    calc_data->code->saved_text_style_blks[0].n_chars = strlen(buf);
    sh_stext_set_style(calc_data->code->saved_text,
		       calc_data->code->saved_text_style_blks, 1);
    rdman_shape_changed(rdman, calc_data->code->saved_text);
}

static void compute(calc_data_t *calc_data, coord_t *tgt) {
    int i;
    coord_t **coord_p;
    redraw_man_t *rdman;
    static int valid_num = 0;
    static int factor = 1;
    static int num = 0;
    static int op = 'n';
    static int saved = 0;
    char buf[2] = " ";

    for(i = 0; i < 16; i++) {
	coord_p = (coord_t **)((void *)calc_data->code + tgt_list[i].off);
	if(*coord_p == (void *)tgt)
	    break;
    }
    if(i >= 16) return;

    if(i < 10) {
	num = num * 10 + i;
	show_text(calc_data, num * factor, saved, op, "");
	valid_num = 1;
    } else {
	switch(tgt_list[i].c) {
	case 'c':
	    saved = num = 0;
	    factor = 1;
	    valid_num = 0;
	    op = 'n';
	    show_text(calc_data, 0, saved, op, "");
	    break;

	case '-':
	    if(!valid_num) {
		factor *= -1;
		valid_num = 1;
		break;
	    }
	case '+':
	case '*':
	case '/':
	    saved = real_compute(op, saved, num * factor);
	    buf[0] = tgt_list[i].c;
	    show_text(calc_data, saved, saved, 'n', buf);
	    op = tgt_list[i].c;
	    num = 0;
	    factor = 1;
	    valid_num = 0;
	    break;

	case '=':
	    saved = real_compute(op, saved, num * factor);
	    show_text(calc_data, saved, 0, 'n', "");
	    num = 0;
	    op = 'N';
	    break;
	}
    }
    rdman = mb_runtime_rdman(calc_data->rt);
    rdman_redraw_changed(rdman);
}

static void buttons_handler(event_t *evt, void *arg) {
    calc_data_t *calc_data = (calc_data_t *)arg;

    switch(evt->type) {
    case EVT_MOUSE_BUT_PRESS:
	compute(calc_data, (coord_t *)evt->cur_tgt->obj);
	break;
    }
}

static void setup_observers(calc_data_t *calc_data) {
    calculator_scr_t *calculator_scr;
    subject_t *subject;
    coord_t *coord;
    int off;
    int i;

    calculator_scr = calc_data->code;

    for(i = 0; i < 16; i++) {
	off = tgt_list[i].off;
	coord = OFF2TYPE(calculator_scr, off, coord_t *);
	subject = coord_get_mouse_event(coord);
	subject_add_observer(subject, buttons_handler, calc_data);
    }
}

int main(int argc, char * const argv[]) {
    mb_rt_t *rt;
    redraw_man_t *rdman;
    calculator_scr_t *calculator_scr;
    calc_data_t calc_data;

    rt = mb_runtime_new(":0.0", 300, 400);

    rdman = mb_runtime_rdman(rt);
    calculator_scr = calculator_scr_new(rdman, rdman->root_coord);

    calc_data.rt = rt;
    calc_data.code = calculator_scr;
    setup_observers(&calc_data);

    mb_runtime_event_loop(rt);

    calculator_scr_free(calculator_scr);
    mb_runtime_free(rt);

    return 0;
}
