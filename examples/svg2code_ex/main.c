/*! \file
 *
 * svg2code_ex is an example that show programmers how to create a
 * menu with MadButterfly.
 *
 */
#include <stdio.h>
#include <mb/mb.h>
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
	rdman_coord_changed(ex_rt->rt->rdman, ex_rt->code->file_menu);
	/* Update changed part to UI. */
	rdman_redraw_changed(ex_rt->rt->rdman);
	break;
    }
}

static void file_menu_handler(event_t *evt, void *arg) {
    ex_rt_t *ex_rt = (ex_rt_t *)arg;

    switch(evt->type) {
    case EVT_MOUSE_BUT_PRESS:
	coord_hide(ex_rt->code->file_menu);
	/* Tell redraw manager that a coord (group) is chagned. */
	rdman_coord_changed(ex_rt->rt->rdman, ex_rt->code->file_menu);
	/* Update changed part to UI. */
	rdman_redraw_changed(ex_rt->rt->rdman);
	break;
    }
}

int main(int argc, char * const argv[]) {
    X_MB_runtime_t rt;
    svg2code_ex_t *svg2code;
    ob_factory_t *factory;
    subject_t *subject;
    ex_rt_t ex_rt;
    int r;

    /*
     * Initialize a runtime with XLib as backend.
     */
    r = X_MB_init(":0.0", 800, 600, &rt);

    /*
     * Instantiate objects from a SVG file.
     */
    svg2code = svg2code_ex_new(rt.rdman);

    /*
     * Get observer factory
     */
    factory = rdman_get_ob_factory(rt.rdman);
    /*
     * Register observers to subjects of events for objects.
     */
    subject = coord_get_mouse_event(svg2code->file_button);
    ex_rt.rt = &rt;
    ex_rt.code = svg2code;
    subject_add_observer(factory, subject, file_button_handler, &ex_rt);
    subject = coord_get_mouse_event(svg2code->file_menu);
    subject_add_observer(factory, subject, file_menu_handler, &ex_rt);

    /*
     * Start handle connections, includes one to X server.
     * User start to interact with the application.
     */
    X_MB_handle_connection(&rt);

    /*
     * Clean
     */
    svg2code_ex_free(svg2code);
    X_MB_destroy(&rt);

    return 0;
}
