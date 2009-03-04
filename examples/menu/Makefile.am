include $(top_srcdir)/config.mk

#SUFFIXES=.svg .so

list.so:list.svg
	$(top_srcdir)/tools/svg2code.py $< $<.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $<.mb > $(<:.svg=.c)
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $<.mb > $(<:.svg=.h)
	$(MAKE) $(<:.svg=.o)
	$(CC) -shared -o $@ $(<:.svg=.o)

browser.so:browser.svg
	$(top_srcdir)/tools/svg2code.py $< $<.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $<.mb > $(<:.svg=.c)
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $<.mb > $(<:.svg=.h)
	$(MAKE) $(<:.svg=.o)
	$(CC) -shared -o $@ $(<:.svg=.o)

noinst_PROGRAMS = menu filebrowser list.so browser.so
EXTRA_DIST = 

menu_SOURCES = main.c animated_menu.c animated_menu.h
nodist_dynamic_SOURCES = 
CFLAGS = @pangocairo_CFLAGS@ 
menu_CFLAGS = @pangocairo_CFLAGS@ 
menu_LDFLAGS = @pangocairo_LIBS@ 
menu_LDADD = $(top_builddir)/src/libmbfly.la
CLEANFILES = menu.mb menu.c menu.h

filebrowser_SOURCES = filebrowser.c animated_menu.c animated_menu.h
filebrowser_CFLAGS = @pangocairo_CFLAGS@ 
filebrowser_LDFLAGS = @pangocairo_LIBS@ 
filebrowser_LDADD = $(top_builddir)/src/libmbfly.la
