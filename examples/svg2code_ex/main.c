#include <stdio.h>
#include <mb_types.h>
#include <X_supp.h>
#include "svg2code_ex.h"

int main(int argc, char * const argv[]) {
    X_MB_runtime_t rt;
    svg2code_ex_t *svg2code;
    int r;

    r = X_MB_init(":0.0", 800, 600, &rt);

    svg2code = svg2code_ex_new(rt.rdman);
    X_MB_handle_connection(rt.display, rt.rdman, rt.tman);

    X_MB_destroy(&rt);

    return 0;
}
