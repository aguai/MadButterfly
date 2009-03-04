include $(top_srcdir)/config.mk

#SUFFIXES=.svg .so

list.so:list.c list.h
	$(MAKE) $(<:.svg=.o)
	$(CC) -shared -o $@ $(<:.svg=.o)

list.mb: $(srcdir)/list.svg
	$(top_srcdir)/tools/svg2code.py $< $<.mb

list.c: list.mb list.h
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $<.mb > $(<:.svg=.c)

list.h: list.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $<.mb > $(<:.svg=.h)

browser.so: browser.c browser.h
	$(MAKE) $(<:.svg=.o)
	$(CC) -shared -o $@ $(<:.svg=.o)

browser.h: browser.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $<.mb > $(<:.svg=.h)

browser.c: browser.mb browser.h
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $<.mb > $(<:.svg=.c)

browser.mb: $(srcdir)/browser.svg
	$(top_srcdir)/tools/svg2code.py $< $<.mb

noinst_PROGRAMS = menu filebrowser list.so browser.so
EXTRA_DIST = 

menu_SOURCES = main.c animated_menu.c animated_menu.h
nodist_menu_SOURCES = 
CFLAGS = @pangocairo_CFLAGS@ 
menu_CFLAGS = @pangocairo_CFLAGS@ 
menu_LDFLAGS = @pangocairo_LIBS@ 
menu_LDADD = $(top_builddir)/src/libmbfly.la
CLEANFILES = menu.mb menu.c menu.h

filebrowser_SOURCES = filebrowser.c animated_menu.c animated_menu.h
filebrowser_CFLAGS = @pangocairo_CFLAGS@ 
filebrowser_LDFLAGS = @pangocairo_LIBS@ 
filebrowser_LDADD = $(top_builddir)/src/libmbfly.la
