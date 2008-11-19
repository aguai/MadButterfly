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
    X_MB_runtime_t *rt;
    svg2code_ex_t *code;
};

static void file_button_handler(event_t *evt, void *arg) {
    ex_rt_t *ex_rt = (ex_rt_t *)arg;

    switch(evt->type) {
    case EVT_MOUSE_BUT_PRESS:
	coord_show(ex_rt->code->file_menu);
	/* Tell redraw manager that a coord (group) is chagned. */
	rdman_coord_changed(X_MB_rdman(ex_rt->rt), ex_rt->code->file_menu);
	/* Update changed part to UI. */
	rdman_redraw_changed(X_MB_rdman(ex_rt->rt));
	break;
    }
}

static void file_menu_handler(event_t *evt, void *arg) {
    ex_rt_t *ex_rt = (ex_rt_t *)arg;
    redraw_man_t *rdman;

    rdman = X_MB_rdman(ex_rt->rt);
    switch(evt->type) {
    case EVT_MOUSE_BUT_PRESS:
	coord_hide(ex_rt->code->file_menu);
	/* Tell redraw manager that a coord (group) is chagned. */
	rdman_coord_changed(rdman, ex_rt->code->file_menu);
	/* Update changed part to UI. */
	rdman_redraw_changed(rdman);
	break;
    }
}

int main(int argc, char * const argv[]) {
    X_MB_runtime_t *rt;
    redraw_man_t *rdman;
    svg2code_ex_t *svg2code;
    subject_t *subject;
    ex_rt_t ex_rt;

    /*
     * Initialize a runtime with XLib as backend.
     */
    rt = X_MB_new(":0.0", 800, 600);

    /*
     * Instantiate objects from a SVG file.
     */
    rdman = X_MB_rdman(rt);
    svg2code = svg2code_ex_new(rdman, rdman->root_coord);

    /*
     * Register observers to subjects of events for objects.
     */
    subject = coord_get_mouse_event(svg2code->file_button);
    ex_rt.rt = rt;
    ex_rt.code = svg2code;
    subject_add_observer(subject, file_button_handler, &ex_rt);
    subject = coord_get_mouse_event(svg2code->file_menu);
    subject_add_observer(subject, file_menu_handler, &ex_rt);

    /*
     * Start handle connections, includes one to X server.
     * User start to interact with the application.
     */
    X_MB_handle_connection(rt);

    /*
     * Clean
     */
    svg2code_ex_free(svg2code);
    X_MB_free(rt);

    return 0;
}
