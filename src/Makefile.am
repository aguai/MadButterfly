include $(top_srcdir)/config.mk

lib_LTLIBRARIES = libmbfly.la

noinst_PROGRAMS =

if SH_TEXT
noinst_PROGRAMS += X_main
endif

if TESTCASE
noinst_PROGRAMS += testcase
endif

if X_BACKEND
MBAF_SOURCES=mbaf/mbapp.c mbaf/mbbutton.c mbaf/mbobject.c mbaf/animated_menu.c
else
MBAF_SOURCES=
endif

libmbfly_la_SOURCES= animate.c chgcolor.c coord.c event.c geo.c	\
	observer.c paint.c redraw_man.c rotate.c shape_path.c		\
	shape_rect.c shift.c subtree_free.c timer.c 			\
	timertool.c tools.c visibility.c prop.c sprite.c	\
	mouse.c shape_image.c img_ldr.c $(MBAF_SOURCES)

if SH_TEXT
libmbfly_la_SOURCES += shape_text.c
endif

if SH_STEXT
libmbfly_la_SOURCES += shape_stext.c
endif

if X_BACKEND
libmbfly_la_SOURCES += X_supp.c
endif

libmbfly_la_LDFLAGS =

if CAIRO_GRAPH_ENGINE
libmbfly_la_SOURCES += graph_engine_cairo.c

libmbfly_la_CPPFLAGS = @cairo_CFLAGS@ @pangocairo_CFLAGS@
libmbfly_la_LDFLAGS += @cairo_LIBS@ @pangocairo_LIBS@
endif

if XSHM
libmbfly_la_LDFLAGS += -lXext
endif

if SKIA_GRAPH_ENGINE
libmbfly_la_SOURCES += graph_engine_skia.cpp
endif

X_main_SOURCES = X_main.c
X_main_LDADD = $(top_builddir)/src/libmbfly.la
X_main_CPPFLAGS = @pangocairo_CFLAGS@ -I$(top_builddir)/include
X_main_LDFLAGS = @pangocairo_LIBS@

testcase_SOURCES = testcase.c $(libmbfly_la_SOURCES)
testcase_CFLAGS = -I$(top_builddir)/include \
		-DUNITTEST -g @cairo_CFLAGS@ \
		@pangocairo_CFLAGS@
testcase_LDFLAGS = -lcunit -L/usr/local/lib/ @pangocairo_LIBS@

EXTRA_PROGRAMS = testcase
