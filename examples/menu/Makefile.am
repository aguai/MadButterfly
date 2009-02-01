include $(top_srcdir)/config.mk

#SUFFIXES=.svg .so

list.so:list.svg
	$(top_srcdir)/tools/svg2code.py $< $<.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $<.mb > $(<:.svg=.c)
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $<.mb > $(<:.svg=.h)
	$(MAKE) $(<:.svg=.o)
	$(CC) -shared -o $@ $(<:.svg=.o)

noinst_PROGRAMS = menu
EXTRA_DIST = 

menu_SOURCES = main.c
nodist_dynamic_SOURCES = 
CFLAGS = @pangocairo_CFLAGS@ 
menu_CFLAGS = @pangocairo_CFLAGS@ 
menu_LDFLAGS = @pangocairo_LIBS@ 
menu_LDADD = $(top_builddir)/src/libmbfly.la
CLEANFILES = menu.mb menu.c menu.h

