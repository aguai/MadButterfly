// -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 4; -*-
// vim: sw=4:ts=8:sts=4
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "mb_graph_engine.h"
#include <pango/pangocairo.h>

#include <string.h>
#include "mb_shapes.h"
#include "mb_redraw_man.h"
#include "mb_paint.h"
#include "mb_timer.h"
#include "mb_animate.h"
#include "mb_backend_utils.h"

#define OK 0
#define ERR -1

Display *display;
Window win;

void hint_shape(redraw_man_t *rdman, shape_t *shape) {
    static shape_t *last_shape = NULL;
    if(last_shape != shape) {
	if(last_shape != NULL && last_shape->stroke != NULL) {
	    last_shape->stroke_width -= 2;
	    rdman_shape_changed(rdman, last_shape);
	}
	if(shape != NULL && shape->stroke != NULL) {
	    shape->stroke_width += 2;
	    rdman_shape_changed(rdman, shape);
	}
    }
    last_shape = shape;
}

void event_interaction(Display *display,
		       redraw_man_t *rdman, int w, int h) {
    XEvent evt;
    XMotionEvent *mevt;
    int r;
    co_aix x, y;
    shape_t *shape = NULL;
    int in_stroke;

    while(XEventsQueued(display, QueuedAfterReading) > 0) {
	r = XNextEvent(display, &evt);
	if(r == -1)
	    break;

	switch(evt.type) {
	case MotionNotify:
	    mevt = (XMotionEvent *)&evt;
	    x = mevt->x;
	    y = mevt->y;
	    shape = find_shape_at_pos(rdman, x, y, &in_stroke);
	    hint_shape(rdman, shape);
	    break;
	case Expose:
	    rdman_redraw_area(rdman, evt.xexpose.x, evt.xexpose.y,
			      evt.xexpose.width, evt.xexpose.height);
	    break;
	}
    }
    rdman_redraw_changed(rdman);
    XFlush(display);
}

void handle_connection(Display *display, mb_tman_t *tman,
		       redraw_man_t *rdman, int w, int h) {
    int xcon;
    fd_set rds;
    int nfds;
    struct timeval tmo;
    mb_timeval_t mb_tmo, next_mb_tmo;
    int r;

    XSelectInput(display, win, PointerMotionMask | ExposureMask);
    XFlush(display);

    xcon = XConnectionNumber(display);
    nfds = xcon + 1;

    r = gettimeofday(&tmo, NULL);
    if(r == -1) {
	perror("gettimeofday");
	return;
    }
    MB_TIMEVAL_SET(&mb_tmo, tmo.tv_sec, tmo.tv_usec);

    while(1) {
	FD_ZERO(&rds);
	FD_SET(xcon, &rds);

	r = mb_tman_next_timeout(tman, &mb_tmo, &next_mb_tmo);
	if(r != OK)
	    r = select(nfds, &rds, NULL, NULL, NULL);
	else {
	    tmo.tv_sec = MB_TIMEVAL_SEC(&next_mb_tmo);
	    tmo.tv_usec = MB_TIMEVAL_USEC(&next_mb_tmo);
	    r = select(nfds, &rds, NULL, NULL, &tmo);
	}

	if(r == -1) {
	    perror("select");
	    return;
	}

	if(r == 0) {
	    MB_TIMEVAL_ADD(&mb_tmo, &next_mb_tmo);

	    mb_tman_handle_timeout(tman, &mb_tmo);
	    rdman_redraw_changed(rdman);
	    XFlush(display);
	} else if(FD_ISSET(xcon, &rds)) {
	    event_interaction(display, rdman, w, h);
	}
    }
}

void draw_path(mbe_t *cr, int w, int h) {
    mbe_t *tmpcr;
    mbe_surface_t *tmpsuf;
    mbe_pattern_t *tmpptn;
    redraw_man_t rdman;
    shape_t *path1, *path2, *rect;
    coord_t *coord1, *coord2, *coord3;
    paint_t *fill1, *fill2, *fill3;
    paint_t *text_fill;
    grad_stop_t text_stops[2];
    paint_t *stroke, *text_stroke;
    shape_t *text;
    grad_stop_t fill3_stops[3];
    mbe_font_face_t *face;
    struct timeval tv;
    mb_timer_man_t *timer_man;
    mb_tman_t *tman;
    mb_timeval_t mbtv, start, playing;
    mb_progm_t *progm;
    mb_word_t *word;
    mb_action_t *act;
    PangoAttrList *attrs = pango_attr_list_new();

    tmpsuf = mbe_image_surface_create(MB_IFMT_ARGB32, w, h);
    tmpptn = mbe_pattern_create_for_surface(tmpsuf);
    tmpcr = mbe_create(tmpsuf);

    mbe_set_source(cr, tmpptn);
    redraw_man_init(&rdman, tmpcr, cr);
    coord1 = rdman_coord_new(&rdman, rdman.root_coord);
    coord2 = rdman_coord_new(&rdman, rdman.root_coord);
    coord3 = rdman_coord_new(&rdman, rdman.root_coord);

    fill1 = rdman_paint_color_new(&rdman, 1, 1, 0, 0.5);
    fill2 = rdman_paint_color_new(&rdman, 0, 1, 1, 0.5);
    stroke = rdman_paint_color_new(&rdman, 0.4, 0.4, 0.4, 1);
    text_stroke = rdman_paint_color_new(&rdman, 0.5, 0.5, 0.5, 1);

    face = mbe_get_font_face(tmpcr);
    text = rdman_shape_text_new(&rdman, "hello \xe6\xbc\xa2\xe5\xad\x97",
				10, h / 4, 36.0, face, attrs);
    text_fill = rdman_paint_radial_new(&rdman, 100, h / 4, 70);
    grad_stop_init(text_stops, 0, 0.2, 0.9, 0.2, 1);
    grad_stop_init(text_stops + 1, 1, 0.9, 0.2, 0.2, 0.1);
    paint_radial_stops(text_fill, 2, text_stops);
    rdman_paint_stroke(&rdman, text_stroke, text);
    text->stroke_width = 0.5;
    rdman_paint_fill(&rdman, text_fill, text);
    rdman_add_shape(&rdman, text, coord3);

    path1 = rdman_shape_path_new(&rdman, "M 22,89.36218 C -34,-0.63782 39,-9.637817 82,12.36218 C 125,34.36218 142,136.36218 142,136.36218 C 100.66667,125.36218 74.26756,123.42795 22,89.36218 z ");
    rdman_paint_fill(&rdman, fill1, path1);
    rdman_paint_stroke(&rdman, stroke, path1);
    coord1->matrix[0] = 0.8;
    coord1->matrix[1] = 0;
    coord1->matrix[2] = 20;
    coord1->matrix[4] = 0.8;
    coord1->matrix[5] = 20;

    path2 = rdman_shape_path_new(&rdman, "M 22,89.36218 C -34,-0.63782 39,-9.637817 82,12.36218 C 125,34.36218 142,136.36218 142,136.36218 C 100.66667,125.36218 74.26756,123.42795 22,89.36218 z ");
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


    fill3 = rdman_paint_linear_new(&rdman, 50, 50, 150, 150);
    grad_stop_init(fill3_stops, 0, 1, 0, 0, 0.5);
    grad_stop_init(fill3_stops + 1, 0.5, 0, 1, 0, 0.5);
    grad_stop_init(fill3_stops + 2, 1, 0, 0, 1, 0.5);
    paint_linear_stops(fill3, 3, fill3_stops);
    rect = rdman_shape_rect_new(&rdman, 50, 50, 100, 100, 20, 20);
    rdman_paint_fill(&rdman, fill3, rect);
    rdman_add_shape(&rdman, (shape_t *)rect, rdman.root_coord);

    rdman_redraw_all(&rdman);

    XFlush(display);

    timer_man = mb_timer_man_new(&tman_timer_factory);
    if(timer_man) {
	tman = mb_tman_new();
	
	/* Prepare an animation program. */
	progm = mb_progm_new(30, &rdman);

	MB_TIMEVAL_SET(&start, 0, 0);
	MB_TIMEVAL_SET(&playing, 1, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(50, 50, coord1, word);
	act = mb_shift_new(-50, 50, coord2, word);

	MB_TIMEVAL_SET(&start, 1, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, 20, coord1, word);
	act = mb_shift_new(0, -20, coord2, word);
	act = mb_visibility_new(VIS_HIDDEN, coord3, word);

	MB_TIMEVAL_SET(&start, 3, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, -20, coord1, word);
	act = mb_shift_new(0, 20, coord2, word);
	act = mb_chgcolor_new(0, 0, 1, 0.5, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 0.5, fill2, word);
	act = mb_visibility_new(VIS_VISIBLE, coord3, word);

	MB_TIMEVAL_SET(&start, 5, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, 20, coord1, word);
	act = mb_shift_new(0, -20, coord2, word);
	act = mb_chgcolor_new(1, 0, 0, 1, fill1, word);
	act = mb_chgcolor_new(0, 0, 1, 1, fill2, word);
	act = mb_visibility_new(VIS_HIDDEN, coord3, word);

	MB_TIMEVAL_SET(&start, 7, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, -20, coord1, word);
	act = mb_shift_new(0, 20, coord2, word);
	act = mb_chgcolor_new(0, 1, 0, 0.5, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 0.5, fill2, word);
	act = mb_visibility_new(VIS_VISIBLE, coord3, word);

	MB_TIMEVAL_SET(&start, 9, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, 20, coord1, word);
	act = mb_shift_new(0, -20, coord2, word);
	act = mb_chgcolor_new(0, 0, 1, 1, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 1, fill2, word);
	act = mb_visibility_new(VIS_HIDDEN, coord3, word);

	MB_TIMEVAL_SET(&start, 11, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, -20, coord1, word);
	act = mb_shift_new(0, 20, coord2, word);
	act = mb_chgcolor_new(0, 1, 0, 0.5, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 0.5, fill2, word);
	act = mb_visibility_new(VIS_VISIBLE, coord3, word);

	MB_TIMEVAL_SET(&start, 13, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, 20, coord1, word);
	act = mb_shift_new(0, -20, coord2, word);
	act = mb_chgcolor_new(0, 0, 1, 1, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 1, fill2, word);
	act = mb_visibility_new(VIS_HIDDEN, coord3, word);

	MB_TIMEVAL_SET(&start, 15, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, -20, coord1, word);
	act = mb_shift_new(0, 20, coord2, word);
	act = mb_chgcolor_new(0, 1, 0, 0.5, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 0.5, fill2, word);
	act = mb_visibility_new(VIS_VISIBLE, coord3, word);

	MB_TIMEVAL_SET(&start, 17, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, 20, coord1, word);
	act = mb_shift_new(0, -20, coord2, word);
	act = mb_chgcolor_new(0, 0, 1, 1, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 1, fill2, word);
	act = mb_visibility_new(VIS_HIDDEN, coord3, word);

	MB_TIMEVAL_SET(&start, 19, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, -20, coord1, word);
	act = mb_shift_new(0, 20, coord2, word);
	act = mb_chgcolor_new(0, 1, 0, 0.5, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 0.5, fill2, word);
	act = mb_visibility_new(VIS_VISIBLE, coord3, word);

	MB_TIMEVAL_SET(&start, 21, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, 20, coord1, word);
	act = mb_shift_new(0, -20, coord2, word);
	act = mb_chgcolor_new(0, 0, 1, 1, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 1, fill2, word);
	act = mb_visibility_new(VIS_HIDDEN, coord3, word);

	MB_TIMEVAL_SET(&start, 23, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, -20, coord1, word);
	act = mb_shift_new(0, 20, coord2, word);
	act = mb_chgcolor_new(0, 1, 0, 0.5, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 0.5, fill2, word);
	act = mb_visibility_new(VIS_VISIBLE, coord3, word);

	MB_TIMEVAL_SET(&start, 25, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, 20, coord1, word);
	act = mb_shift_new(0, -20, coord2, word);
	act = mb_chgcolor_new(0, 0, 1, 1, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 1, fill2, word);
	act = mb_visibility_new(VIS_HIDDEN, coord3, word);

	MB_TIMEVAL_SET(&start, 27, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, -20, coord1, word);
	act = mb_shift_new(0, 20, coord2, word);
	act = mb_chgcolor_new(0, 1, 0, 0.5, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 0.5, fill2, word);
	act = mb_visibility_new(VIS_VISIBLE, coord3, word);

	MB_TIMEVAL_SET(&start, 29, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, 20, coord1, word);
	act = mb_shift_new(0, -20, coord2, word);
	act = mb_chgcolor_new(0, 0, 1, 1, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 1, fill2, word);
	act = mb_visibility_new(VIS_HIDDEN, coord3, word);

	MB_TIMEVAL_SET(&start, 31, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, -20, coord1, word);
	act = mb_shift_new(0, 20, coord2, word);
	act = mb_chgcolor_new(0, 1, 0, 0.5, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 0.5, fill2, word);
	act = mb_visibility_new(VIS_VISIBLE, coord3, word);

	MB_TIMEVAL_SET(&start, 33, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, 20, coord1, word);
	act = mb_shift_new(0, -20, coord2, word);
	act = mb_chgcolor_new(0, 0, 1, 1, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 1, fill2, word);
	act = mb_visibility_new(VIS_HIDDEN, coord3, word);

	MB_TIMEVAL_SET(&start, 35, 0);
	MB_TIMEVAL_SET(&playing, 2, 0);
	word = mb_progm_next_word(progm, &start, &playing);

	act = mb_shift_new(0, -20, coord1, word);
	act = mb_shift_new(0, 20, coord2, word);
	act = mb_chgcolor_new(0, 1, 0, 0.5, fill1, word);
	act = mb_chgcolor_new(1, 0, 0, 0.5, fill2, word);
	act = mb_visibility_new(VIS_VISIBLE, coord3, word);

	/* Start playing the program. */
	gettimeofday(&tv, NULL);
	MB_TIMEVAL_SET(&mbtv, tv.tv_sec, tv.tv_usec);
	mb_progm_start(progm, timer_man, &mbtv);

	handle_connection(display, tman, &rdman, w, h);

	mb_progm_free(progm);
	mb_timer_man_free(&tman_timer_factory, timer_man);
    }

    rdman_paint_free(&rdman, fill1);
    rdman_paint_free(&rdman, fill2);
    rdman_paint_free(&rdman, stroke);
    rdman_paint_free(&rdman, text_stroke);
    rdman_paint_free(&rdman, text_fill);
    rdman_shape_free(&rdman, path1);
    rdman_shape_free(&rdman, path2);
    rdman_shape_free(&rdman, rect);
    rdman_shape_free(&rdman, text);
    redraw_man_destroy(&rdman);
    mbe_destroy(tmpcr);
    mbe_surface_destroy(tmpsuf);
}

void drawing(mbe_surface_t *surface, int w, int h) {
    mbe_t *cr;

    cr = mbe_create(surface);
    mbe_set_source_rgb(cr, 0, 0, 0);
    mbe_paint(cr);
    draw_path(cr, w, h);
    mbe_destroy(cr);
}

int
main(int argc, char * const argv[]) {
    Window root;
    Visual *visual;
    int screen;
    XSetWindowAttributes wattr;
    int depth;
    mbe_surface_t *surface;
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

    surface = mbe_xlib_surface_create(display, win, visual, w, h);
    if(surface == NULL)
	printf("mbe_xlib_surface_create\n");

    drawing(surface, w, h);

    XFlush(display);
    sleep(10);

    XCloseDisplay(display);
    return 0;
}
