include $(top_srcdir)/config.mk

noinst_PROGRAMS = ex1
EXTRA_DIST = menu.svg

ex1_SOURCES = main.c
nodist_ex1_SOURCES = menu.c menu.h
ex1_CPPFLAGS = @cairo_CFLAGS@ -I$(top_srcdir)
ex1_LDFLAGS = @cairo_LIBS@ 
ex1_LDADD = $(top_builddir)/src/libmbfly.la
BUILT_SOURCES = menu.c menu.h menu.mb
CLEANFILES = menu.c menu.h menu.mb

menu.mb: $(srcdir)/menu.svg
	$(top_srcdir)/tools/svg2code.py $? $@

menu.h: menu.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

menu.c: menu.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@
