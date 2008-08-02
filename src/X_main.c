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
    shape_t *path1, *path2, *path3;
    coord_t *coord1, *coord2;
    paint_t *fill1, *fill2, *fill3;
    paint_t *stroke;
    grad_stop_t fill3_stops[3];
    int i;

    redraw_man_init(&rdman, cr);
    coord1 = rdman_coord_new(&rdman, rdman.root_coord);
    coord2 = rdman_coord_new(&rdman, rdman.root_coord);

    fill1 = paint_color_new(&rdman, 1, 1, 0, 0.5);
    fill2 = paint_color_new(&rdman, 0, 1, 1, 0.5);
    stroke = paint_color_new(&rdman, 0.4, 0.4, 0.4, 1);
    path1 = sh_path_new("M 22,89.36218 C -34,-0.63782 39,-9.637817 82,12.36218 C 125,34.36218 142,136.36218 142,136.36218 C 100.66667,125.36218 74.26756,123.42795 22,89.36218 z ");
    rdman_paint_fill(&rdman, fill1, path1);
    rdman_paint_stroke(&rdman, stroke, path1);
    coord1->matrix[0] = 0.8;
    coord1->matrix[1] = 0;
    coord1->matrix[2] = 20;
    coord1->matrix[4] = 0.8;
    coord1->matrix[5] = 20;
    path2 = sh_path_new("M 22,89.36218 C -34,-0.63782 39,-9.637817 82,12.36218 C 125,34.36218 142,136.36218 142,136.36218 C 100.66667,125.36218 74.26756,123.42795 22,89.36218 z ");
    rdman_paint_fill(&rdman, fill2, path2);
    rdman_paint_stroke(&rdman, stroke, path2);
    coord2->matrix[0] = -0.8;
    coord2->matrix[1] = 0;
    coord2->matrix[2] = 180;
    coord2->matrix[4] = 0.8;
    coord2->matrix[5] = 20;
    rdman_coord_changed(&rdman, coord1);
    rdman_coord_changed(&rdman, coord2);
    rdman_add_shape(&rdman, (shape_t *)path1, coord1);
    rdman_add_shape(&rdman, (shape_t *)path2, coord2);

    
    fill3 = paint_linear_new(&rdman, 50, 50, 150, 150);
    grad_stop_init(fill3_stops, 0, 1, 0, 0, 0.5);
    grad_stop_init(fill3_stops + 1, 0.5, 0, 1, 0, 0.5);
    grad_stop_init(fill3_stops + 2, 1, 0, 0, 1, 0.5);
    paint_linear_stops(fill3, 3, fill3_stops);
    path3 = sh_path_new("M 50,50 L 50,150 L 150,150 L 150,50 z");
    rdman_paint_fill(&rdman, fill3, path3);
    rdman_add_shape(&rdman, (shape_t *)path3, rdman.root_coord);

    rdman_redraw_all(&rdman);

    XFlush(display);

    for(i = 0; i < 50; i++) {
	usleep(20000);
	coord1->matrix[2] += 1;
	coord1->matrix[5] += 1;
	coord2->matrix[2] -= 1;
	coord2->matrix[5] += 1;
	paint_color_set(fill1, 1, 1, (i/25) & 0x1, 0.5);
	paint_color_set(fill2, (i/25) & 0x1, 1, 1, 0.5);
	rdman_paint_changed(&rdman, fill1);
	rdman_paint_changed(&rdman, fill2);
	rdman_coord_changed(&rdman, coord1);
	rdman_coord_changed(&rdman, coord2);
	rdman_redraw_changed(&rdman);
	XFlush(display);
    }

    for(i = 0; i < 5; i++) {
	usleep(500000);
	paint_color_set(fill1, 1, i % 2, 0, 0.5);
	paint_color_set(fill2, 0, i % 2, 1, 0.5);
	rdman_paint_changed(&rdman, fill1);
	rdman_paint_changed(&rdman, fill2);
	rdman_redraw_changed(&rdman);
	XFlush(display);
    }

    fill1->free(fill1);
    fill2->free(fill2);
    redraw_man_destroy(&rdman);
    sh_path_free(path1);
    sh_path_free(path2);
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
