#include <stdio.h>
#include <mb_types.h>
#include <X_supp.h>
#include "svg2code_ex.h"

typedef struct _ex_rt ex_rt_t;
struct _ex_rt {
    X_MB_runtime_t *rt;
    svg2code_ex_t *code;
};

static void file_button_handler(event_t *evt, void *arg) {
    ex_rt_t *ex_rt = (ex_rt_t *)arg;

    coord_show(ex_rt->code->file_menu);
    rdman_coord_changed(ex_rt->rt->rdman, ex_rt->code->file_menu);
    rdman_redraw_changed(ex_rt->rt->rdman);
}

int main(int argc, char * const argv[]) {
    X_MB_runtime_t rt;
    svg2code_ex_t *svg2code;
    ob_factory_t *factory;
    subject_t *subject;
    ex_rt_t ex_rt;
    int r;

    r = X_MB_init(":0.0", 800, 600, &rt);

    svg2code = svg2code_ex_new(rt.rdman);

    factory = rdman_get_ob_factory(rt.rdman);
    subject = coord_get_mouse_event(svg2code->file_button);
    ex_rt.rt = &rt;
    ex_rt.code = svg2code;
    subject_add_observer(factory, subject, file_button_handler, &ex_rt);

    X_MB_handle_connection(&rt);

    svg2code_ex_free(svg2code);
    X_MB_destroy(&rt);

    return 0;
}
