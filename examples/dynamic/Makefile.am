include $(top_srcdir)/config.mk

noinst_PROGRAMS = dynamic
EXTRA_DIST = menu.svg button.svg

dynamic_SOURCES = main.c
nodist_dynamic_SOURCES = menu.c menu.h button.c button.h
dynamic_CPPFLAGS = @cairo_CFLAGS@ -I$(top_srcdir)
dynamic_LDFLAGS = @cairo_LIBS@ 
dynamic_LDADD = $(top_builddir)/src/libmbfly.la
BUILT_SOURCES = menu.c menu.h menu.mb
CLEANFILES = menu.c menu.h menu.mb

menu.mb: $(srcdir)/menu.svg
	$(top_srcdir)/tools/svg2code.py $? $@

menu.h: menu.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

menu.c: menu.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

button.mb: $(srcdir)/button.svg
	$(top_srcdir)/tools/svg2code.py $? $@

button.h: button.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

button.c: button.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@
