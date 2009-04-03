include $(top_srcdir)/config.mk

lib_LTLIBRARIES = libmbfly.la

noinst_PROGRAMS = X_main
MBAF_SOURCES=mbaf/mbapp.c mbaf/mbbutton.c mbaf/mbobject.c mbaf/animated_menu.c
libmbfly_la_SOURCES = animate.c chgcolor.c coord.c event.c geo.c	\
	observer.c paint.c redraw_man.c rotate.c shape_path.c		\
	shape_rect.c shape_text.c shift.c subtree_free.c timer.c 	\
	timertool.c tools.c visibility.c X_supp.c prop.c sprite.c	\
	mouse.c shape_image.c img_ldr.c $(MBAF_SOURCES)

libmbfly_la_CPPFLAGS = @cairo_CFLAGS@ `pkg-config --cflags pangocairo`
libmbfly_la_LDFLAGS = @cairo_LIBS@ `pkg-config --libs pangocairo`

X_main_SOURCES = X_main.c
X_main_LDADD = $(top_builddir)/src/libmbfly.la
X_main_CPPFLAGS = @pangocairo_CFLAGS@ -I$(top_builddir)/src
X_main_LDFLAGS = @pangocairo_LIBS@
