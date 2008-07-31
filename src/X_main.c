#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include <string.h>
#include "shapes.h"
#include "redraw_man.h"

void draw_path(cairo_t *cr, int w, int h) {
    redraw_man_t rdman;
    shape_t *path;
    coord_t *coord;

    redraw_man_init(&rdman);
    coord = rdman.root_coord;

    path = sh_path_new("M 22,89.36218 C -34,-0.63782 39,-9.637817 82,12.36218 C 125,34.36218 142,136.36218 142,136.36218 C 100.66667,125.36218 74.26756,123.42795 22,89.36218 z ");
    coord->aggr_matrix[0] = 0.8;
    coord->aggr_matrix[1] = 0;
    coord->aggr_matrix[2] = 20;
    coord->aggr_matrix[4] = 0.8;
    coord->aggr_matrix[5] = 20;
    rdman_add_shape(&rdman, (shape_t *)path, coord);
    sh_path_transform(path);
    sh_path_draw(path, cr);
    sh_path_free(path);
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
    cairo_set_font_size(cr, 36.0);
    cairo_text_path(cr, "hello \xe6\xbc\xa2\xe5\xad\x97");
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
