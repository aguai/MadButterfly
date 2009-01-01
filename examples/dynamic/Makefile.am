include $(top_srcdir)/config.mk

noinst_PROGRAMS = dynamic button.so scene.so
EXTRA_DIST = menu.svg button.svg

dynamic_SOURCES = main.c
nodist_dynamic_SOURCES = menu.c menu.h menu.mb
CPPFLAGS = @cairo_CFLAGS@ $(INCLUDES)
dynamic_LDFLAGS = @cairo_LIBS@ 
dynamic_LDADD = $(top_builddir)/src/libmbfly.la
BUILT_SOURCES = menu.c menu.h menu.mb button.c button.h button.mb
CLEANFILES = menu.c menu.h menu.mb button.c button.h button.mb

menu.mb: $(srcdir)/menu.svg
	$(top_srcdir)/tools/svg2code.py $? $@

menu.h: menu.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

menu.c: menu.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

button.so: button.o button.h
	gcc -shared -o button.so button.o

button.mb: $(srcdir)/button.svg
	$(top_srcdir)/tools/svg2code.py $? $@

button.h: button.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

button.c: button.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

scene.so: scene.o scene.h
	gcc -shared -o scene.so scene.o

scene.mb: $(srcdir)/scene.svg
	$(top_srcdir)/tools/svg2code.py $? $@

scene.h: scene.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

scene.c: scene.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@
