/*! \file
 *
 * svg2code_ex is an example that show programmers how to create a
 * menu with MadButterfly.
 *
 */
#include <stdio.h>
#include <mb.h>
#include "svg2code_ex.h"

typedef struct _ex_rt ex_rt_t;
struct _ex_rt {
    mb_rt_t *rt;
    svg2code_ex_t *code;
};

#define COORD_SHOW(group) do {						\
	coord_show(group);						\
	rdman_coord_changed(mb_runtime_rdman(ex_rt->rt), group);	\
    } while(0)
#define COORD_HIDE(group) do {						\
	coord_hide(group);						\
	rdman_coord_changed(mb_runtime_rdman(ex_rt->rt), group);	\
    } while(0)

static void file_button_handler(event_t *evt, void *arg) {
    ex_rt_t *ex_rt = (ex_rt_t *)arg;
    redraw_man_t *rdman;

    COORD_SHOW(ex_rt->code->file_menu);
    rdman = mb_runtime_rdman(ex_rt->rt);
    /* Update changed part to UI. */
    rdman_redraw_changed(rdman);
}

static void file_menu_handler(event_t *evt, void *arg) {
    ex_rt_t *ex_rt = (ex_rt_t *)arg;
    redraw_man_t *rdman;

    COORD_HIDE(ex_rt->code->file_menu);
    rdman = mb_runtime_rdman(ex_rt->rt);
    /* Update changed part to UI. */
    rdman_redraw_changed(rdman);
}

int main(int argc, char * const argv[]) {
    mb_rt_t *rt;
    redraw_man_t *rdman;
    svg2code_ex_t *svg2code;
    subject_t *subject;
    ex_rt_t ex_rt;

    /*
     * Initialize a runtime with XLib as backend.
     */
    rt = mb_runtime_new(":0.0", 800, 600);

    /*
     * Instantiate objects from a SVG file.
     */
    rdman = mb_runtime_rdman(rt);
    svg2code = svg2code_ex_new(rdman, rdman->root_coord);

    /*
     * Register observers to subjects of events for objects.
     */
    subject = coord_get_mouse_event(svg2code->file_button);
    ex_rt.rt = rt;
    ex_rt.code = svg2code;
    subject_add_event_observer(subject,  EVT_MOUSE_BUT_PRESS, file_button_handler, &ex_rt);
    subject = coord_get_mouse_event(svg2code->file_menu);
    subject_add_event_observer(subject,  EVT_MOUSE_BUT_PRESS,  file_menu_handler, &ex_rt);

    /*
     * Start handle connections, includes one to X server.
     * User start to interact with the application.
     */
    mb_runtime_event_loop(rt);

    /*
     * Clean
     */
    svg2code_ex_free(svg2code);
    mb_runtime_free(rt);

    return 0;
}
