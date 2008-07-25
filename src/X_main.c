#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include <string.h>
#include "shapes.h"

void draw_path(cairo_t *cr, int w, int h) {
    shape_t *path;
    coord_t coord;

    path = sh_path_new("m80 80 c 20 5 -30 20 10 30 t -30 0 z");
    memset(coord.aggr_matrix, 0, sizeof(co_aix) * 6);
    coord.aggr_matrix[0] = 1;
    coord.aggr_matrix[1] = 0.5;
    coord.aggr_matrix[2] = -30;
    coord.aggr_matrix[4] = 1;
    coord.aggr_matrix[5] = -20;
    sh_path_transform(path, &coord);
    sh_path_draw(path, cr);
}

void drawing(cairo_surface_t *surface, int w, int h) {
    cairo_t *cr;

    cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 0.9, 0.1, 0.1);
    draw_path(cr, w, h);
    cairo_set_source_rgb(cr, 0.5, 0.9, 0.8);
    cairo_move_to(cr, 10, h / 2);
    cairo_set_font_size(cr, 48.0);
    cairo_text_path(cr, "hello \xe4\xb8\xad\xe6\x96\x87");
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);
}

int
main(int argc, char * const argv[]) {
    Display *display;
    Window root;
    Visual *visual;
    int screen;
    XSetWindowAttributes wattr;
    Window win;
    int depth;
    cairo_surface_t *surface;
    int w, h;
    int x, y;
    int r;

    display = XOpenDisplay(":0.0");
    if(display == NULL)
	printf("XOpenDisplay\n");
    screen = DefaultScreen(display);
    root = DefaultRootWindow(display);
    visual = DefaultVisual(display, screen);
    depth = DefaultDepth(display, screen);
    wattr.override_redirect = False;
    x = 10;
    y = 10;
    w = 200;
    h = 200;
    win = XCreateWindow(display, root,
			x, y,
			w, h,
			1, depth, InputOutput, visual,
			CWOverrideRedirect, &wattr);
    r = XMapWindow(display, win);

    surface = cairo_xlib_surface_create(display, win, visual, w, h);
    if(surface == NULL)
	printf("cairo_xlib_surface_create\n");

    drawing(surface, w, h);

    XFlush(display);
    sleep(10);

    XCloseDisplay(display);
    return 0;
}
