include $(top_srcdir)/config.mk

#SUFFIXES=.svg .mbso

noinst_PROGRAMS = dynamic hello text
noinst_LTLIBRARIES = button.la mytext.la scene.la
EXTRA_DIST = menu.svg button.svg

dynamic_SOURCES = main.c
nodist_dynamic_SOURCES = 
CPPFLAGS = @pangocairo_CFLAGS@ $(INCLUDES)
dynamic_LDFLAGS = @pangocairo_LIBS@ 
dynamic_LDADD = $(top_builddir)/src/libmbfly.la
BUILT_SOURCES = 
CLEANFILES = menu.c menu.h menu.mb \
	button.c button.h button.mb \
	scene.c scene.h scene.mb


hello_SOURCES = hello.c 
hello_LDFLAGS = @pangocairo_LIBS@ 
hello_LDADD = $(top_builddir)/src/libmbfly.la


text_SOURCES = text.c 
text_LDFLAGS = @pangocairo_LIBS@ 
text_LDADD = $(top_builddir)/src/libmbfly.la


button_la_SOURCES = button.c
button_la_LDFLAGS = -module -avoid-version -rpath /nowhere -shared


mytext_la_SOURCES = mytext.c
mytext_la_LDFLAGS = -module -avoid-version -rpath /nowhere -shared


scene_la_SOURCES = scene.c
scene_la_LDFLAGS = -module -avoid-version -rpath /nowhere -shared


button.c: button.mb button.h
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

button.h: button.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

button.mb: button.svg
	$(top_srcdir)/tools/svg2code.py $< $@

mytext.c: mytext.mb mytext.h
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

mytext.h: mytext.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

mytext.mb: mytext.svg
	$(top_srcdir)/tools/svg2code.py $< $@

scene.c: scene.mb scene.h
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

scene.h: scene.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

scene.mb: scene.svg
	$(top_srcdir)/tools/svg2code.py $< $@

