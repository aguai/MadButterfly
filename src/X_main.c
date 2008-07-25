#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <cairo.h>

void drawing(cairo_surface_t *surface, int w, int h) {
    cairo_t *cr;

    cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 0xff, 0xff, 0x80);
    cairo_move_to(cr, 10, h / 2);
    cairo_set_font_size(cr, 48.0);
    cairo_text_path(cr, "hello \xe4\xb8\xad\xe6\x96\x87");
    cairo_set_line_width(cr, 1);
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
}
