include_HEADERS = animate.h mb.h mb_timer.h mb_types.h observer.h paint.h redraw_man.h shapes.h tools.h X_supp.h

lib_LTLIBRARIES = libmbfly.la

bin_PROGRAMS = X_main

libmbfly_la_SOURCES = animate.c chgcolor.c coord.c event.c geo.c observer.c paint.c redraw_man.c rotate.c shape_path.c shape_rect.c shape_text.c shift.c timer.c timertool.c tools.c visibility.c X_supp.c
libmbfly_la_CPPFLAGS = @cairo_CFLAGS@
libmbfly_la_LDFLAGS = @cairo_LIBS@

X_main_SOURCES = X_main.c
X_main_LDADD = $(top_builddir)/src/libmbfly.la
X_main_CPPFLAGS = @cairo_CFLAGS@ -I$(top_builddir)/src
X_main_LDFLAGS = @cairo_LIBS@
