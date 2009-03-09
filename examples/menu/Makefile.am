include $(top_srcdir)/config.mk

#SUFFIXES=.svg .so

list.so: list.o
	$(CC) $(CFLAGS) -I ../../include -shared -o $@ $<

list.o: list.h

list.mb: $(srcdir)/list.svg
	$(top_srcdir)/tools/svg2code.py $< $@

list.c: list.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

list.h: list.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

browser.so: browser.o
	$(CC) $(CFLAGS) -I../../include  -I ../../include -shared -o $@ $<

browser.o: browser.h

browser.h: browser.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

browser.c: browser.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@
browser.mb: $(srcdir)/browser.svg
	$(top_srcdir)/tools/svg2code.py $< $@

noinst_PROGRAMS = menu filebrowser list.so browser.so
EXTRA_DIST = 

menu_SOURCES = main.c animated_menu.c animated_menu.h
nodist_menu_SOURCES = 
CFLAGS = @pangocairo_CFLAGS@ -g
menu_CFLAGS = @pangocairo_CFLAGS@ 
menu_LDFLAGS = @pangocairo_LIBS@ 
menu_LDADD = $(top_builddir)/src/libmbfly.la
CLEANFILES = menu.mb menu.c menu.h \
	list.mb list.c list.h \
	browser.mb browser.c browser.h

filebrowser_SOURCES = filebrowser.c animated_menu.c animated_menu.h
filebrowser_CFLAGS = @pangocairo_CFLAGS@ 
filebrowser_LDFLAGS = @pangocairo_LIBS@ 
filebrowser_LDADD = $(top_builddir)/src/libmbfly.la
