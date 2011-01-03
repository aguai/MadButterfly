include $(top_srcdir)/config.mk

#SUFFIXES=.svg .so

noinst_LTLIBRARIES = list.la browser.la
list_la_SOURCES = list.c
list_la_LDFLAGS = -module -avoid-version -rpath /nowhere -shared
browser_la_SOURCES = browser.c
browser_la_LDFLAGS = -module -avoid-version -rpath /nowhere -shared

list.mb: $(srcdir)/list.svg
	$(top_srcdir)/tools/svg2code.py $< $@

list.c: list.mb list.h
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

list.h: list.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

browser.h: browser.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

browser.c: browser.mb browser.h
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

browser.mb: $(srcdir)/browser.svg
	$(top_srcdir)/tools/svg2code.py $< $@

noinst_PROGRAMS = menu filebrowser
EXTRA_DIST = 

menu_SOURCES = main.c
nodist_menu_SOURCES = 
CFLAGS = @pangocairo_CFLAGS@ -g
menu_CFLAGS = @pangocairo_CFLAGS@ 
menu_LDFLAGS = @pangocairo_LIBS@ 
menu_LDADD = $(top_builddir)/src/libmbfly.la
CLEANFILES = menu.mb menu.c menu.h \
	list.mb list.c list.h \
	browser.mb browser.c browser.h

filebrowser_SOURCES = filebrowser.c
filebrowser_CFLAGS = @pangocairo_CFLAGS@ 
filebrowser_LDFLAGS = @pangocairo_LIBS@ 
filebrowser_LDADD = $(top_builddir)/src/libmbfly.la
