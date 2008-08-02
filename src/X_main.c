#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include <string.h>
#include "shapes.h"
#include "redraw_man.h"
#include "paint.h"

Display *display;

void draw_path(cairo_t *cr, int w, int h) {
    redraw_man_t rdman;
    shape_t *path;
    coord_t *coord;
    paint_t *fill;
    int i;

    redraw_man_init(&rdman, cr);
    coord = rdman.root_coord;

    fill = paint_color_new(&rdman, 1, 1, 0);
    path = sh_path_new("M 22,89.36218 C -34,-0.63782 39,-9.637817 82,12.36218 C 125,34.36218 142,136.36218 142,136.36218 C 100.66667,125.36218 74.26756,123.42795 22,89.36218 z ");
    rdman_paint_fill(&rdman, fill, path);
    coord->matrix[0] = 0.8;
    coord->matrix[1] = 0;
    coord->matrix[2] = 20;
    coord->matrix[4] = 0.8;
    coord->matrix[5] = 20;
    rdman_coord_changed(&rdman, coord);
    rdman_add_shape(&rdman, (shape_t *)path, coord);

    rdman_redraw_all(&rdman);

    XFlush(display);

    for(i = 0; i < 50; i++) {
	usleep(20000);
	coord->matrix[2] += 1;
	coord->matrix[5] += 1;
	paint_color_set(fill, 1, 1, (i/25) & 0x1);
	rdman_coord_changed(&rdman, coord);
	rdman_redraw_changed(&rdman);
	XFlush(display);
    }

    for(i = 0; i < 5; i++) {
	usleep(500000);
	paint_color_set(fill, 1, i % 2, 0);
	rdman_paint_changed(&rdman, fill);
	rdman_redraw_changed(&rdman);
	XFlush(display);
    }

    fill->free(fill);
    redraw_man_destroy(&rdman);
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
