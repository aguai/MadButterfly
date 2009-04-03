include $(top_srcdir)/config.mk

#SUFFIXES=.svg .mbso

noinst_PROGRAMS = dynamic hello text button.so mytext.so scene.so
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

button.so: button.o
	$(CC) $(CFLAGS) -I ../../include -shared -o $@ $<

button.o: button.c button.h

button.c: button.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

button.h: button.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

button.mb: button.svg
	$(top_srcdir)/tools/svg2code.py $< $@

mytext.so: mytext.o
	$(CC) $(CFLAGS) -I ../../include -shared -o $@ $<

mytext.o: mytext.c mytext.h

mytext.c: mytext.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

mytext.h: mytext.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

mytext.mb: mytext.svg
	$(top_srcdir)/tools/svg2code.py $< $@

scene.so: scene.o
	$(CC) $(CFLAGS) -I ../../include -shared -o $@ $<

scene.o: scene.c scene.h

scene.c: scene.mb
	m4 -I $(top_srcdir)/tools mb_c_source.m4 $< > $@

scene.h: scene.mb
	m4 -I $(top_srcdir)/tools mb_c_header.m4 $< > $@

scene.mb: scene.svg
	$(top_srcdir)/tools/svg2code.py $< $@

