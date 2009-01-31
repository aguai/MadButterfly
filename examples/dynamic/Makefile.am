include $(top_srcdir)/config.mk

SUFFIXES=.svg .so

%.so:%.svg
	$(top_srcdir)/tools/svg2code.py $< $<.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $<.mb > $(<:.svg=.c)
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $<.mb > $(<:.svg=.h)
	make $(<:.svg=.o)
	gcc -shared -o $@ $(<:.svg=.o)

noinst_PROGRAMS = dynamic hello list
EXTRA_DIST = menu.svg button.svg

dynamic_SOURCES = main.c mbapp.c mbapp.h mbbutton.c mbbutton.h
nodist_dynamic_SOURCES = 
CPPFLAGS = @pangocairo_CFLAGS@ $(INCLUDES)
dynamic_LDFLAGS = @pangocairo_LIBS@ 
dynamic_LDADD = $(top_builddir)/src/libmbfly.la
CLEANFILES = menu.c menu.h menu.mb button.c button.o button.mb button.so


hello_SOURCES = hello.c mbapp.c mbapp.h
hello_LDFLAGS = @pangocairo_LIBS@ 
hello_LDADD = $(top_builddir)/src/libmbfly.la

list_SOURCES = list.c mbapp.c mbapp.h
list_LDFLAGS = @pangocairo_LIBS@ 
list_LDADD = $(top_builddir)/src/libmbfly.la


